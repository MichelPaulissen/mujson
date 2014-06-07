#ifndef MUJSON_H_INCLUDED
#define MUJSON_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifndef MUJSON_NO_HIGH_LEVEL_FUNCTIONS
#include <stdio.h>
#else
// #warning MUJSON_NO_HIGH_LEVEL_FUNCTIONS defined
#endif

#ifdef MUJSON_USE_CPP_INTERFACE
#define MUJSON_MANUAL_STREAM
#define MUJSON_NO_HIGH_LEVEL_FUNCTIONS
#endif

#define MUJ_UNUSED(x) (void)(x)

#ifndef MUJ_INDEX
#define MUJ_INDEX uint32_t
#endif

#if 0
Usage:

	FILE* f = fopen("file.json", "ro");
	size_t original_json_size = file_size(f);
	muj_compressed_json target = allocate_json_target(original_json_size);
	muj_source source; // This can be your own code, see MUJSON_MANUAL_STREAM
	source.file = f;
	size_t stats_size;
	muj_document_stats stats = muj_make_document_stats(&stats_size);
	muj_phase1(source, target, stats);
	
	muj_document_table table = muj_make_document_table(*stats.table_size);
	
	muj_document document = muj_make_document(target, table);
	
	muj_phase2(document);
	
	// From here on you can do processing.
	// 	MUJ_INDEX root = muj_get_root_object(document.table);
	// ...
#endif

typedef struct
{
	MUJ_INDEX key;
	MUJ_INDEX value;
} muj_key_value_pair;

typedef struct
{
	char* json_target;
	size_t json_max_size;
	size_t* json_write_pos;
	size_t* json_read_pos;
	size_t* table_size;
} muj_compressed_json;

typedef struct
{
	MUJ_INDEX* table;
	size_t* current_write_pos;
	size_t table_size_in_indices;
} muj_document_table;

typedef struct
{
	muj_compressed_json json;
	muj_document_table table;
} muj_document;

#ifndef MUJSON_NO_HIGH_LEVEL_FUNCTIONS
muj_document muj_load_document_from_file(FILE* f);
void muj_unload_document(muj_document document);
#endif

#ifndef MUJSON_MANUAL_STREAM

typedef struct
{
	FILE *file;
} muj_source;

static bool muj_read_byte(muj_source source, char* byte)
{
	int c = getc(source.file);
	if (c != EOF)
	{
		*byte = (char)c;
		return true;
	}
	return false;
}

static bool muj_peek_byte(muj_source source, char* byte)
{
	bool success = muj_read_byte(source, byte);
	if (success)
	{
		ungetc(*byte, source.file);
	}
	return success;
}

#endif

const char* muj_get_last_error();
size_t muj_get_string_length(MUJ_INDEX string, muj_document document);
void muj_copy_string(char* target, MUJ_INDEX string, muj_document document);
size_t muj_object_count_number_of_children(MUJ_INDEX object, muj_document document);
size_t muj_array_count_number_of_elements(MUJ_INDEX array, muj_document document);
void muj_object_copy_children(muj_key_value_pair* children, MUJ_INDEX object, muj_document document);
void muj_array_copy_elements(MUJ_INDEX* children, MUJ_INDEX array, muj_document document);
MUJ_INDEX muj_get_root_object(muj_document_table table);
long muj_get_long(MUJ_INDEX number, muj_document document);
double muj_get_double(MUJ_INDEX number, muj_document document);
MUJ_INDEX muj_find_value_of_key_in_object(MUJ_INDEX object, char* key, muj_document document); // slow if used more than once
MUJ_INDEX muj_get_element_from_array(MUJ_INDEX array, size_t index, muj_document document); // slow if used more than once

bool muj_is_object(MUJ_INDEX index, muj_document document);
bool muj_is_array(MUJ_INDEX index, muj_document document);
bool muj_is_string(MUJ_INDEX index, muj_document document);
bool muj_is_boolean(MUJ_INDEX index, muj_document document);
bool muj_is_true(MUJ_INDEX index, muj_document document);
bool muj_is_false(MUJ_INDEX index, muj_document document);
bool muj_is_null(MUJ_INDEX index, muj_document document);
bool muj_is_constant(MUJ_INDEX index, muj_document document);
bool muj_is_number(MUJ_INDEX index, muj_document document);
bool muj_is_object_empty(MUJ_INDEX object, muj_document document);
bool muj_is_array_empty(MUJ_INDEX array, muj_document document);

muj_document_table muj_allocate_document_table(muj_compressed_json what_for);
void muj_free_document_table(muj_document_table table);
muj_compressed_json muj_allocate_compressed_json(size_t uncompressedSizeInBytes);
void muj_free_compressed_json(muj_compressed_json json);
muj_document muj_make_document(muj_compressed_json json, muj_document_table table);

void muj_phase1(muj_source source, muj_compressed_json target);
void muj_phase2( muj_document document);

// Generic usage functions
char* muj_alloc_string_copy_target(MUJ_INDEX string, muj_document document);
char* muj_alloc_string_copy_target_and_copy(MUJ_INDEX string, muj_document document);
void muj_free_string_copy_target(char* string);

#endif // MUJSON_H_INCLUDED
