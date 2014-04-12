#include <mujson.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef MUJSON_NO_SETJMP
#include <setjmp.h>
#endif

// Why another json parser?
// 	This is mostly just to freshen up my C.
//	It might be very obvious from the code I don't use pure C that much.

// mujson is a two-phase, low cpu usage, small memory footprint single C99 file json parser and it works as follows:
// 1. It starts streaming the content from the source. While streaming, it does as follows:
// 	a. Copy over the json excluding whitespace, newlines and commas ("compression")
// 	b. Count the number of key/value pairs in the document in order to make the smallest possible document table allocation
// 2. Allocate the document table

// It has the following combination of attributes:
// - It is not a verifier; it may incorrectly parse bogus data as correct.
// - It seperates the parsing phase (which works with streams) from the tree building phase
// - The parse result uses (practically always, with a max 1 byte overhead) less memory than the size of the json file
// - The parse result and table are serial data, so can for example be written to disk
// - The parse result has the same characters as the input JSON, so can be used in environments where no binary data is allowed without modification
//		- However, this does mean that unicode cannot be supported; input data in the '\u1234' form will become 'u1234'
// - It requires only 2 dynamic memory allocations: For the compressed document text, and for the document table.
// - It doesn't deduct value types during the parsing phase. Hence the use case is (partial) single parse of a json file.
// - The internal data layout is optimal for depth-first parsing
// - The parse phase does a single linear (streaming) scan of the input json

#ifndef MUJSON_MALLOC
#define MUJSON_MALLOC(x) malloc(x)
#endif

#ifndef MUJSON_FREE
#define MUJSON_FREE(x) free(x)
#endif

#ifndef MUJSON_ASSERT
#define MUJSON_ASSERT(x) assert(x)
#endif

#define MUJSON_SINGLE_MALLOC

const char* muj_problem_string = 0;

const char* muj_get_last_error()
{
	const char* last_error = muj_problem_string;
	muj_problem_string = 0;
	return last_error;
}

#ifndef MUJSON_NO_SETJMP
static jmp_buf problem_jmp_buf;
#define MUJ_PROBLEM(string) \
muj_problem_string = string;\
printf("mujson: Problem occured: %s", string);\
longjmp(problem_jmp_buf, 1)
#else
#define MUJ_PROBLEM(string) muj_problem_string = string; printf("mujson: Problem occured, bad things may happen: %s", string)
#endif

muj_document muj_make_document(muj_compressed_json json, muj_document_table table)
{
	muj_document document;
	document.json = json;
	document.table = table;
	return document;
}

#ifndef MUJSON_NO_HIGH_LEVEL_FUNCTIONS

unsigned int file_size(FILE* f)
{
	unsigned int pos = ftell(f);
	fseek(f, 0, SEEK_END);
	unsigned int end = ftell(f);
	fseek(f, pos, SEEK_SET);
	
	return end;
}

muj_document muj_load_document_from_file(FILE* f)
{
	size_t original_json_size = file_size(f);
	muj_compressed_json target = muj_allocate_compressed_json(original_json_size);
	muj_source source;
	source.file = f;
	muj_phase1(source, target);
	
	muj_document_table table = muj_allocate_document_table(target);
	
	muj_document document = muj_make_document(target, table);
	
	muj_phase2(document);
	
	return document;
}

void muj_unload_document(muj_document document)
{
	muj_free_compressed_json(document.json);
	muj_free_document_table(document.table);
}

#endif

void muj_increase_table_size(muj_compressed_json json)
{
	MUJSON_ASSERT(json.table_size);
	(*json.table_size) += 2;
}

void muj_expect_byte(muj_source source, char expectation)
{
	char byte = 0;
	bool success = muj_read_byte(source, &byte);
	if (!success || (success && (byte != expectation)))
	{
		if (!success)
			printf("Failed reading byte (EOF?)\n");
		else
			printf("Expected similarity: %d ('%c') should be %d ('%c')\n", (int)byte, byte, (int)expectation, expectation);
		MUJ_PROBLEM("Unexpected data in phase 1.\n");
	}
	
}

bool is_whitespace(char byte)
{
	return (byte <= ' '); // All ascii characters below (space) are either whitespace or non-renderable
}

void skip_whitespace(muj_source source)
{
	char byte;
	while(true)
	{
		bool success = muj_peek_byte(source, &byte);
		if (!success)
			break;
		if (is_whitespace(byte))
		{
			muj_read_byte(source, &byte); // will always succeed because peek did
		}
		else
			break;
	}
}

muj_compressed_json muj_allocate_compressed_json(size_t uncompressedSizeInBytes)
{
	size_t bytes = uncompressedSizeInBytes+1; // Worst case inflation is 1 byte: [1,1] -> [+1+1]
	muj_compressed_json out;
	out.json_max_size = 0;
#ifdef MUJSON_SINGLE_MALLOC
	size_t malloc_size = bytes + sizeof(size_t)*3;
	out.json_target = (char*)MUJSON_MALLOC(malloc_size);
	out.json_write_pos = (size_t*)((char*)out.json_target +(long) malloc_size - (long)sizeof(size_t)*3);
	out.json_read_pos = (size_t*)((char*)out.json_target + (long)malloc_size - (long)sizeof(size_t)*2);
	out.table_size = (size_t*)((char*)out.json_target + (long)malloc_size - (long)sizeof(size_t)*1);
#else
	size_t malloc_size = bytes;
	out.json_target = MUJSON_MALLOC(malloc_size);
	out.json_write_pos = MUJSON_MALLOC(sizeof(size_t));
	out.json_read_pos = MUJSON_MALLOC(sizeof(size_t));
	out.table_size = MUJSON_MALLOC(sizeof(size_t));
#endif
	*out.json_write_pos = 0;
	*out.json_read_pos = 0;
	*out.table_size = 0;
	if (out.json_target != NULL)
		out.json_max_size = bytes;
	return out;
}

void muj_free_compressed_json(muj_compressed_json json)
{
	MUJSON_FREE(json.json_target);
#ifndef MUJSON_SINGLE_MALLOC
	MUJSON_FREE(json.json_write_pos);
	MUJSON_FREE(json.json_read_pos);
#endif
}

// Key: String
// Value: Object, Array, String or constant (null, false, true)
/*
MUJ_INDEX constant_cost = (3*sizeof(MUJ_INDEX)); // pos, len, skip
MUJ_INDEX object_cost = (); // num keys, pos, len, */

void push_byte_to_target(muj_compressed_json target, char byte)
{
	if ((*target.json_write_pos) >= target.json_max_size)
	{
		printf("Target size: %d\n", (int)target.json_max_size);
		printf("Write attempt: %d\n", (int)(*target.json_write_pos));
		MUJ_PROBLEM("Compressed JSON target not large enough.\n");
	}
	else
	{
		target.json_target[(*target.json_write_pos)++] = byte;
	}
}	

void muj_phase1_value_constant(muj_source source, muj_compressed_json target)
{
	char byte = 0;
	bool success = muj_read_byte(source, &byte);
	if (success)
	{
		push_byte_to_target(target, byte);
		for(int i=1; i<4; i++)
		{
			bool success = muj_read_byte(source, &byte);
			if (!success)
				goto FAIL_CONSTANT_PARSING;
		}
		if (byte == 's')
		{
			bool success = muj_read_byte(source, &byte); // e of false
			if (!success)
				goto FAIL_CONSTANT_PARSING;
		}
	}
	else
	{
FAIL_CONSTANT_PARSING:
		printf("Failed reading byte (EOF?)\n");
		MUJ_PROBLEM("EOF in phase 1.\n");
	}
}

void skip_string(muj_source source, muj_compressed_json target)
{
	char byte = 0;
	muj_expect_byte(source, '"'); // '"'
	push_byte_to_target(target, '"');
	while(true)
	{
		bool success = muj_read_byte(source, &byte);
		if (success)
		{
			push_byte_to_target(target, byte);
			if (byte == '"')
				break;
			else if (byte == '\\')
			{
				bool success = muj_read_byte(source, &byte);
				if (success)
				{
					push_byte_to_target(target, byte);
				}
				else
				{
					goto FAIL_STRING_SKIPPPING;
				}
			}
		}
		else
		{
FAIL_STRING_SKIPPPING:
			printf("Failed reading byte (EOF?)\n");
			MUJ_PROBLEM("EOF in string parsing.\n");
			break;
		}
	}
}

bool isByteDigit(char byte)
{
	return (byte >= '0' && byte <= '9');
}

bool isByteExponent(char byte)
{
	return (byte == 'e' || byte == 'E');
}

bool isByteNumber(char byte, bool includingSign)
{
	if (includingSign)
		return (isByteDigit(byte) || byte == '.' || byte == '-' || byte == '+' || isByteExponent(byte));
	return (isByteDigit(byte) || byte == '.' || isByteExponent(byte));
}

void skip_number(muj_source source, muj_compressed_json target)
{
	char byte = 0;
	muj_peek_byte(source, &byte);
	if (byte != '-' && byte != '+')
		push_byte_to_target(target, '+');
	else
	{
		bool success = muj_read_byte(source, &byte);
		if (success)
		{
			push_byte_to_target(target, byte);
		}
		else
		{
			printf("Failed reading byte (EOF?)\n");
			MUJ_PROBLEM("EOF in number parsing.\n");
			return;
		}
	}
	bool byte_was_e = false;
	while(true)
	{
		bool success = muj_peek_byte(source, &byte);
		if (success)
		{
			if (isByteNumber(byte, byte_was_e))
			{
				muj_read_byte(source, &byte); // will always succeed because peek did
				
				bool byte_is_e = isByteExponent(byte);
				
				if (byte_is_e)
				{
					char next_byte;
					bool epeek_success = muj_peek_byte(source, &next_byte);
					if (!epeek_success)
						goto SKIP_NUMBER_PEEK_FAILED;
					
					if (next_byte == '-')
					{
						push_byte_to_target(target, 'e'); // negative
						muj_read_byte(source, &byte);
						//push_byte_to_target(target, byte);
						byte_is_e = false;
					}
					else
					{
						push_byte_to_target(target, 'E'); // positive
						if (next_byte == '+')
						{
							muj_read_byte(source, &byte);
							//push_byte_to_target(target, byte);
							byte_is_e = false;
						}
					}
				}
				else
					push_byte_to_target(target, byte);
				
				if (byte_was_e)
				{
					byte_was_e = false;
				}
				else
				{
					byte_was_e = byte_is_e;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
SKIP_NUMBER_PEEK_FAILED:
			printf("Failed reading byte (EOF?)\n");
			MUJ_PROBLEM("EOF in number parsing.\n");
			return;
		}
	}
}

void muj_phase1_value_string(muj_source source, muj_compressed_json target)
{
	skip_string(source, target);
}

void muj_phase1_value_number(muj_source source, muj_compressed_json target)
{
	skip_number(source, target);
}

void skip_assignment(muj_source source)
{
	muj_expect_byte(source, ':'); // ':' or '='
}

void muj_phase1_key(muj_source source, muj_compressed_json target)
{
	skip_string(source, target);
	skip_whitespace(source);
	skip_assignment(source);
	skip_whitespace(source);
}

void muj_phase1_value(muj_source source, muj_compressed_json target);

void muj_phase1_value_object(muj_source source, muj_compressed_json target)
{
	char byte = 0;
	muj_expect_byte(source, '{'); // '{'
	push_byte_to_target(target, '{');
	bool in_object = true;
	while(in_object)
	{
		skip_whitespace(source);
		muj_peek_byte(source, &byte); // '}' or '"'
		switch(byte)
		{
			case '}':
			{
				muj_expect_byte(source, '}');
				push_byte_to_target(target, '}');
				in_object = false;
				break;
			}
			case '"':
			{
				
				muj_phase1_key(source, target);
				muj_phase1_value(source, target);
				skip_whitespace(source);
				muj_peek_byte(source, &byte);
				muj_increase_table_size(target);
				muj_increase_table_size(target);
				if (byte == ',')
				{
					muj_expect_byte(source, ','); // skip comma
				}

				break;
			}
			default:
			{
				printf("Unexpected byte: %d\n", (int)byte);
				MUJ_PROBLEM("Unexpected data in phase 1. (Expected object continuation)\n");
			}
		}
	}
}

void muj_phase1_value_array(muj_source source, muj_compressed_json target)
{
	char byte = 0;
	muj_expect_byte(source, '['); // '['
	push_byte_to_target(target, '[');
	bool in_array = true;
	while(in_array)
	{
		skip_whitespace(source);
		muj_phase1_value(source, target);
		skip_whitespace(source);
		muj_peek_byte(source, &byte);
		
		muj_increase_table_size(target);
		
		if(byte == ',')
		{
			muj_expect_byte(source, ',');
		}
		else if (byte == ']')
		{
			muj_expect_byte(source, ']');
			push_byte_to_target(target, ']');
			in_array = false;
		}
		else
		{
			printf("Unexpected byte: %d\n", byte);
			MUJ_PROBLEM("Unexpected data in phase 1. (Expected array continuation)\n");
		}
	}
}

void muj_phase1_value(muj_source source, muj_compressed_json target)
{
	char byte = 0;
	muj_peek_byte(source, &byte);
	switch(byte)
	{
		case 'n': case 't': case 'f':
			muj_phase1_value_constant(source, target);
			break;
		case '{':
			muj_phase1_value_object(source, target);
			break;
		case '[':
			muj_phase1_value_array(source, target);
			break;
		case '"':
			muj_phase1_value_string(source, target);
			break;
		default:
			muj_phase1_value_number(source, target);
			break;
	}
}

void muj_phase1(muj_source source, muj_compressed_json target)
{
#ifndef MUJSON_NO_SETJMP
	if (!setjmp(problem_jmp_buf))
	{
		muj_increase_table_size(target);
		skip_whitespace(source);
		muj_phase1_value(source, target);
	}
#else
	muj_increase_table_size(target);
	skip_whitespace(source);
	muj_phase1_value(source, target);
#endif
}

muj_document_table muj_allocate_document_table(muj_compressed_json what_for)
{
	size_t indices = *what_for.table_size;
	muj_document_table out;
#ifdef MUJSON_SINGLE_MALLOC
	size_t malloc_size = indices * sizeof(MUJ_INDEX) + sizeof(size_t);
	out.table = (uint32_t*)MUJSON_MALLOC(malloc_size);
	out.current_write_pos = (size_t*)((char*)out.table + malloc_size - (long)sizeof(size_t));
#else
	size_t malloc_size = indices * sizeof(MUJ_INDEX);
	out.table = MUJSON_MALLOC(malloc_size);
	out.current_write_pos = MUJSON_MALLOC(sizeof(size_t));
#endif
	out.table_size_in_indices = out.table!=0?indices:0;
	*out.current_write_pos = 0;
	return out;
}

void muj_free_document_table(muj_document_table table)
{
	MUJSON_FREE(table.table);
#ifndef MUJSON_SINGLE_MALLOC
	MUJSON_FREE(table.current_write_pos);
#endif
}

MUJ_INDEX muj_push_index_to_table(muj_document_table table, MUJ_INDEX index)
{
	size_t pos = (*table.current_write_pos);
	if (pos >= table.table_size_in_indices)
	{
		printf("Table size: %d\n", (int)table.table_size_in_indices);
		printf("Write attempt: %d\n", (int)(*table.current_write_pos));
		MUJ_PROBLEM("Table not large enough.\n");
		return pos;
	}
	else
	{
		(*table.current_write_pos)++;
		table.table[pos] = index;
		return pos;
	}
}

MUJ_INDEX muj_push_current_index_to_table(muj_document document)
{
	return muj_push_index_to_table(document.table, *document.json.json_read_pos);
}

void muj_replace_index_in_table(muj_document_table table, MUJ_INDEX pos_in_table, MUJ_INDEX new_index)
{
	if (pos_in_table >= table.table_size_in_indices)
	{
		printf("Table size: %d\n", (int)table.table_size_in_indices);
		printf("Write attempt: %d\n", (int)(*table.current_write_pos));
		MUJ_PROBLEM("Table not large enough.\n");
	}
	else
	{
		table.table[pos_in_table] = new_index;
	}
}

char peek_json_byte(muj_compressed_json json)
{
	if ((*json.json_read_pos) >= json.json_max_size)
	{
		printf("Compressed JSON size: %d\n", (int)json.json_max_size);
		printf("Peek attempt: %d\n", (int)(*json.json_read_pos));
		MUJ_PROBLEM("Peek would cause a segfault.\n");
		return 0;
	}
	else
	{
		return json.json_target[*json.json_read_pos];
	}
}

char read_json_byte(muj_compressed_json json)
{
	if ((*json.json_read_pos) >= json.json_max_size)
	{
		printf("Compressed JSON size: %d\n", (int)json.json_max_size);
		printf("Read attempt: %d\n", (int)(*json.json_read_pos));
		MUJ_PROBLEM("Read would cause a segfault.\n");
		return 0;
	}
	else
	{
		char byte = json.json_target[*json.json_read_pos];
		(*json.json_read_pos)++;
		return byte;
	}
}

void muj_phase2_value_constant(muj_document document)
{
	read_json_byte(document.json);
}

void muj_phase2_skip_string(muj_document document)
{
	read_json_byte(document.json);
	while(true)
	{
		char byte = read_json_byte(document.json);
		if (byte == '"')
			break;
		else if (byte == '\\')
		{
			read_json_byte(document.json);
		}
	}
}

void muj_phase2_skip_number(muj_document document)
{
	read_json_byte(document.json);
	bool byte_was_e = false;
	while(true)
	{
		char byte = peek_json_byte(document.json);
		if (!isByteNumber(byte, byte_was_e))
			break;
		read_json_byte(document.json);
		
		if (byte_was_e)
		{
			byte_was_e = false;
		}
		else
		{
			byte_was_e = isByteExponent(byte);
		}
	}
}

void muj_phase2_key(muj_document document)
{
	muj_phase2_skip_string(document);
}

void muj_phase2_value(muj_document document);

void muj_phase2_value_object(muj_document document)
{
	read_json_byte(document.json); // {
	bool in_object = true;
	while(in_object)
	{
		char byte = peek_json_byte(document.json);
		switch(byte)
		{
			case '}':
			{
				read_json_byte(document.json);
				in_object = false;
				break;
			}
			case '"':
			{
				muj_push_current_index_to_table(document);
				MUJ_INDEX skip_replace_me_after = muj_push_current_index_to_table(document);
				muj_phase2_key(document);
				muj_replace_index_in_table(document.table, skip_replace_me_after, *document.table.current_write_pos);
				muj_push_current_index_to_table(document);
				skip_replace_me_after = muj_push_current_index_to_table(document);
				muj_phase2_value(document);
				char end_peek = peek_json_byte(document.json);
				if (end_peek == '}')
					muj_replace_index_in_table(document.table, skip_replace_me_after, 0);
				else
					muj_replace_index_in_table(document.table, skip_replace_me_after, *document.table.current_write_pos);
				break;
			}
			default:
			{
				printf("Unexpected byte: %d\n", (int)byte);
				MUJ_PROBLEM("Unexpected data in phase 2.\n");
			}
		}
	}
}

void muj_phase2_value_array(muj_document document)
{
	read_json_byte(document.json); // [
	bool in_array = true;
	while(in_array)
	{
		char byte = peek_json_byte(document.json); 
		if (byte == ']')
		{
			read_json_byte(document.json);
			in_array = false;
		}
		else
		{
			muj_push_current_index_to_table(document);
			MUJ_INDEX skip_replace_me_after = muj_push_current_index_to_table(document);
			muj_phase2_value(document);
			char end_peek = peek_json_byte(document.json);
			if (end_peek == ']')
				muj_replace_index_in_table(document.table, skip_replace_me_after, 0);
			else
				muj_replace_index_in_table(document.table, skip_replace_me_after, *document.table.current_write_pos);
		}
	}
}

void muj_phase2_value_string(muj_document document)
{
	muj_phase2_skip_string(document);
}

void muj_phase2_value_number(muj_document document)
{
	muj_phase2_skip_number(document);
}

void muj_phase2_value(muj_document document)
{
	char byte = peek_json_byte(document.json);
	
	switch(byte)
	{
		case 'n': case 't': case 'f':
			muj_phase2_value_constant(document);
			break;
		case '{':
			muj_phase2_value_object(document);
			break;
		case '[':
			muj_phase2_value_array(document);
			break;
		case '"':
			muj_phase2_value_string(document);
			break;
		default:
			muj_phase2_value_number(document);
			break;
	}
}

void muj_phase2( muj_document document)
{
#ifndef MUJSON_NO_SETJMP
	if (!setjmp(problem_jmp_buf))
	{
		muj_push_current_index_to_table(document);
		muj_push_current_index_to_table(document);
		muj_phase2_value(document);
	}
#else
	// root object (at 0), skip 0 (end after)
	muj_push_current_index_to_table(document);
	muj_push_current_index_to_table(document);
	muj_phase2_value(document);
#endif
}

char get_json_identifier(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	return document.json.json_target[document.table.table[index]];
}

bool muj_is_object(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	return (get_json_identifier(index, document) == '{');
}

bool muj_is_array(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	return (get_json_identifier(index, document) == '[');
}

bool muj_is_string(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	return (get_json_identifier(index, document) == '"');
}

bool muj_is_boolean(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	char id = get_json_identifier(index, document);
	return (id == 't' || id == 'f');
}

bool muj_is_true(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	return (get_json_identifier(index, document) == 't');
}

bool muj_is_false(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	return (get_json_identifier(index, document) == 'f');
}

bool muj_is_null(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	return (get_json_identifier(index, document) == 'n');
}

bool muj_is_constant(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	char id = get_json_identifier(index, document);
	return (id == 't' || id == 'f' || id == 'n');
}

bool muj_is_number(MUJ_INDEX index, muj_document document)
{
	MUJSON_ASSERT(index < document.table.table_size_in_indices);
	char id = get_json_identifier(index, document);
	return (id == '+' || id == '-');
}

bool muj_is_object_empty(MUJ_INDEX object, muj_document document)
{
	MUJSON_ASSERT(object < document.table.table_size_in_indices);
	return (document.json.json_target[document.table.table[object]+1] == '}');
}

bool muj_is_array_empty(MUJ_INDEX array, muj_document document)
{
	MUJSON_ASSERT(array < document.table.table_size_in_indices);
	return (document.json.json_target[document.table.table[array]+1] == ']');
}

MUJ_INDEX get_skip(MUJ_INDEX skip, muj_document_table table)
{
	MUJSON_ASSERT(skip < table.table_size_in_indices);
	return table.table[skip];
}

bool skip_end(MUJ_INDEX skip, muj_document_table table)
{
	MUJSON_ASSERT(skip < table.table_size_in_indices);
	return (get_skip(skip, table) == 0);
}

MUJ_INDEX object_get_first_child(MUJ_INDEX object, muj_document document)
{
	MUJSON_ASSERT(object < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_object(object, document));
	MUJSON_ASSERT(!muj_is_object_empty(object, document));
	return object+2;
}

MUJ_INDEX array_get_first_child(MUJ_INDEX array, muj_document document)
{
	MUJSON_ASSERT(array < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_array(array, document));
	MUJSON_ASSERT(!muj_is_array_empty(array, document));
	return array+2;
}

size_t muj_get_string_length_excluding_null(MUJ_INDEX string, muj_document document)
{
	MUJSON_ASSERT(string < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_string(string, document));
	char* str = (&document.json.json_target[document.table.table[string]])+1;
	size_t len = 0;
	while(*str != '"')
	{
		if (*str == '\\')
		{
			str++;
		}
		len++;
		str++;
	}
	return len;
}

size_t muj_get_string_length(MUJ_INDEX string, muj_document document)
{
	return muj_get_string_length_excluding_null(string, document)+1;
}

void muj_copy_string(char* target, MUJ_INDEX string, muj_document document)
{
	MUJSON_ASSERT(string < document.table.table_size_in_indices);
	MUJSON_ASSERT(target);
	MUJSON_ASSERT(muj_is_string(string, document));
	
	char* str = (&document.json.json_target[document.table.table[string]])+1;
	while(*str != '"')
	{
		if (*str == '\\')
		{
			str++;
		}
		*target = *str;
		str++;
		target++;
	}
}

bool muj_compare_string(MUJ_INDEX string_in_document, char* comparison, muj_document document)
{
	MUJSON_ASSERT(string_in_document < document.table.table_size_in_indices);
	MUJSON_ASSERT(comparison);
	MUJSON_ASSERT(muj_is_string(string_in_document, document));
	
	char* str = (&document.json.json_target[document.table.table[string_in_document]])+1;
	while(*str != '"')
	{
		if (*str == '\\')
		{
			str++;
		}
		if ( *comparison != *str)
			return false;
		str++;
		comparison++;
	}
	return true;
}

size_t muj_get_reparsed_number_length_including_null(MUJ_INDEX number, muj_document document)
{
	MUJSON_ASSERT(number < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_number(number, document));
	char* str = (&document.json.json_target[document.table.table[number]]);
	for( int l=0; true; l++)
	{
		if (!isByteNumber(str[l], l==0))
			return l + 2; // 1 for expansion of 'e' and 1 for \0
	}
}

void muj_reparse_number(char* target, char* str)
{
	for(int i=0; true; i++)
	{
		char byte = str[i];
		if (isByteNumber(byte, i == 0))
		{
			if (isByteExponent(byte))
			{
				*target = 'e';
				target++;
				*target = (byte=='e'?'-':'+');
			}
			else
			{
				*target = byte;
			}
			target++;
		}
		else
			return;
	}
}

long muj_get_long(MUJ_INDEX number, muj_document document)
{
	MUJSON_ASSERT(number < document.table.table_size_in_indices); 
	MUJSON_ASSERT(muj_is_number(number, document)); 
	size_t number_length = muj_get_reparsed_number_length_including_null(number, document); 
	char* number_string = (char*)malloc(number_length);
	number_string[number_length-1] = 0; number_string[number_length-2] = 0; 
	char* str = (&document.json.json_target[document.table.table[number]]); 
	muj_reparse_number(number_string, str);
	long out = strtol(number_string, NULL, 10);
	free(number_string);
	return out;
}

double muj_get_double(MUJ_INDEX number, muj_document document)
{
	MUJSON_ASSERT(number < document.table.table_size_in_indices); 
	MUJSON_ASSERT(muj_is_number(number, document)); 
	size_t number_length = muj_get_reparsed_number_length_including_null(number, document); 
	char* number_string = (char*)malloc(number_length);
	number_string[number_length-1] = 0; number_string[number_length-2] = 0; 
	char* str = (&document.json.json_target[document.table.table[number]]); 
	muj_reparse_number(number_string, str);
	double out = strtod(number_string, NULL);
	free(number_string);
	return out;
}

MUJ_INDEX muj_find_value_of_key_in_object(MUJ_INDEX object, char* key, muj_document document)
{
	MUJSON_ASSERT(object < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_object(object, document));
	if (muj_is_object_empty(object, document))
		return 0;
	MUJ_INDEX child = object_get_first_child(object, document); // key
	if (muj_compare_string(child, key, document))
	{
		if (skip_end(child+1, document.table)) // This should never happen if the data is correct
			return 0;
		return get_skip(child+1, document.table); // value
	}
	while(!skip_end(child+1, document.table))
	{
		child = get_skip(child+1, document.table); // value
		if (skip_end(child+1, document.table))
			return 0;
		child = get_skip(child+1, document.table); // key
		
		if (muj_compare_string(child, key, document))
		{
			if (skip_end(child+1, document.table)) // This should never happen if the data is correct
				return 0;
			return get_skip(child+1, document.table); // value
		}
	}
	return 0;
}

size_t muj_object_count_number_of_children(MUJ_INDEX object, muj_document document)
{
	MUJSON_ASSERT(object < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_object(object, document));
	if (muj_is_object_empty(object, document))
		return 0;
	MUJ_INDEX child = object_get_first_child(object, document);
	size_t numChildren = 1;
	while(!skip_end(child+1, document.table))
	{
		child = get_skip(child+1, document.table);
		numChildren++;
	}
	MUJSON_ASSERT((numChildren & 1) == 0);
	return numChildren/2;
}

size_t muj_array_count_number_of_elements(MUJ_INDEX array, muj_document document)
{
	MUJSON_ASSERT(array < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_array(array, document));
	if (muj_is_array_empty(array, document))
		return 0;
	MUJ_INDEX child = array_get_first_child(array, document);
	size_t numChildren = 1;
	while(!skip_end(child+1, document.table))
	{
		child = get_skip(child+1, document.table);
		numChildren++;
	}
	return numChildren;
}

MUJ_INDEX muj_get_element_from_array(MUJ_INDEX array, size_t index, muj_document document)
{
	MUJSON_ASSERT(array < document.table.table_size_in_indices);
	MUJSON_ASSERT(muj_is_array(array, document));
	if (muj_is_array_empty(array, document))
		return 0;
	MUJ_INDEX child = array_get_first_child(array, document);
	if (index == 0)
		return child;
	size_t current = 1;
	while(!skip_end(child+1, document.table))
	{
		child = get_skip(child+1, document.table);
		if (current == index)
			return child;
		current++;
	}
	return 0;
}

void muj_object_copy_children(muj_key_value_pair* children, MUJ_INDEX object, muj_document document)
{
	MUJSON_ASSERT(object < document.table.table_size_in_indices);
	MUJSON_ASSERT(children);
	MUJSON_ASSERT(muj_is_object(object, document));
	if (muj_is_object_empty(object, document))
		return;
	MUJ_INDEX child = object_get_first_child(object, document);
	(*children).key = child;
	MUJSON_ASSERT(muj_is_string((*children).key, document));
	(*children).value = child+2;
	MUJSON_ASSERT(get_skip((*children).key + 1, document.table) == (*children).value);
	while(!skip_end((*children).value + 1, document.table))
	{
		child = get_skip((*children).value + 1, document.table);
		children = &children[1]; // children++;
		(*children).key = child;
		MUJSON_ASSERT(muj_is_string((*children).key, document));
		(*children).value = child+2;
	}
}

void muj_array_copy_elements(MUJ_INDEX* children, MUJ_INDEX array, muj_document document)
{
	MUJSON_ASSERT(array < document.table.table_size_in_indices);
	MUJSON_ASSERT(children);
	MUJSON_ASSERT(muj_is_array(array, document));
	if (muj_is_array_empty(array, document))
		return;
	*children = array_get_first_child(array, document);
	while(!skip_end((*children)+1, document.table))
	{
		MUJ_INDEX skip = (*children)+1;
		children = &children[1];
		(*children) = get_skip(skip, document.table);
	}
}

MUJ_INDEX muj_get_root_object(muj_document_table table)
{
	MUJ_UNUSED(table);
	return 0;
}

#if 0

void print_string(MUJ_INDEX string, muj_document document)
{
	size_t strlen = get_string_length(string, json, table);
	char str[strlen];
	str[strlen-1] = 0;
	copy_string(str, string, json, table);
	printf("%s\n", str);
}

void test()
{
	FILE* f = fopen("sprites.json", "ro");
	size_t size = file_size(f);
	muj_compressed_json target = allocate_json_target(size);
	muj_source source;
	source.file = f;
	muj_document_stats stats = muj_make_document_stats();
	muj_phase1(source, target);
	
	muj_document_table table = muj_make_document_table(*stats.table_size);
	
	printf("Size is %dK\n", (unsigned)(*target.json_write_pos/1024));
	printf("Table size is %d\n", (unsigned)(*stats.table_size));
	
	muj_phase2(table, target);
#if 1
	printf("Table write pos: %d Table size: %d\n", (unsigned)*table.current_write_pos, (unsigned)*stats.table_size);
	
	FILE* c = fopen("cjson.txt", "wb");
	fwrite(target.json_target, 1, *target.json_write_pos, c);
	fclose(c);

	FILE* t = fopen("table.txt", "wb");
	
	for( int i=0; i<*table.current_write_pos; i+=2)
	{
		fprintf(t, "%d %d\n", table.table[i], table.table[i+1]);
	}
	fclose(t);
#endif
#if 1
	MUJSON_ASSERT(muj_is_object(get_root_object(table), target, table));
	size_t numChildren = object_count_number_of_children(get_root_object(table), target, table);
	printf("Number of children for root: %d\n", (unsigned)numChildren);
	muj_key_value_pair root_children[numChildren];
	object_copy_children(root_children, get_root_object(table), target, table);
	size_t name_len = get_string_length(root_children[0].key, target, table);
	char name[name_len];
	name[name_len-1] = 0;
	copy_string(name, root_children[0].key, target, table);
	printf("Name: %s\n", name);
	MUJSON_ASSERT(muj_is_array(root_children[0].value, target, table));

	MUJ_INDEX array = root_children[0].value;
	size_t num_group_children = array_count_number_of_elements(array, target, table);
	MUJ_INDEX array_elements[num_group_children];
	array_copy_elements(array_elements, array, target, table);
	
	for( int i=0; i<num_group_children; i++)
	{
		MUJ_INDEX object = array_elements[i];
		size_t num_object_children = object_count_number_of_children(object, target, table);
		muj_key_value_pair object_children[num_object_children];
		object_copy_children(object_children, object, target, table);
		MUJ_INDEX name = object_children[0].value;
		size_t name_length = get_string_length(name, target, table);
		char obj_name[name_length];
		copy_string(obj_name, name, target, table);
		obj_name[name_length-1] = 0;
		printf("Name: %s\n", obj_name);
	}
#endif
	
}

#endif
