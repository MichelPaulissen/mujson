#include <stdio.h>
#include <string.h>

#include <mujson.h>



char * files[] = {
"../../test/regular.json",
"../../test/ac_difficult_json_c_test_case_with_comments.json",
"../../test/ac_simple_with_comments.json",
"../../test/ag_false_then_garbage.json",
"../../test/ag_null_then_garbage.json",
"../../test/ag_true_then_garbage.json",
"../../test/am_eof.json",
"../../test/am_integers.json",
"../../test/am_multiple.json",
"../../test/am_stuff.json",
"../../test/ap_array_open.json",
"../../test/ap_eof_str.json",
"../../test/ap_map_open.json",
"../../test/ap_partial_ok.json",
"../../test/array.json",
"../../test/array_close.json",
"../../test/bignums.json",
"../../test/bogus_char.json",
"../../test/codepoints_from_unicode_org.json",
"../../test/deep_arrays.json",
"../../test/difficult_json_c_test_case.json",
"../../test/doubles.json",
"../../test/doubles_in_array.json",
"../../test/empty_array.json",
"../../test/empty_string.json",
"../../test/escaped_bulgarian.json",
"../../test/escaped_foobar.json",
"../../test/false.json",
"../../test/fg_false_then_garbage.json",
"../../test/fg_issue_7.json",
"../../test/fg_null_then_garbage.json",
"../../test/fg_true_then_garbage.json",
"../../test/four_byte_utf8.json",
"../../test/high_overflow.json",
"../../test/integers.json",
"../../test/invalid_utf8.json",
"../../test/isolated_surrogate_marker.json",
"../../test/leading_zero_in_number.json",
"../../test/lonely_minus_sign.json",
"../../test/lonely_number.json",
"../../test/low_overflow.json",
"../../test/map_close.json",
"../../test/missing_integer_after_decimal_point.json",
"../../test/missing_integer_after_exponent.json",
"../../test/multiple.json",
"../../test/non_utf8_char_in_string.json",
"../../test/np_partial_bad.json",
"../../test/null.json",
"../../test/nulls_and_bools.json",
"../../test/simple.json",
"../../test/simple_with_comments.json",
"../../test/string_invalid_escape.json",
"../../test/string_invalid_hex_char.json",
"../../test/string_with_escapes.json",
"../../test/string_with_invalid_newline.json",
"../../test/three_byte_utf8.json",
"../../test/true.json",
"../../test/unescaped_bulgarian.json",
"../../test/zerobyte.json"
};

unsigned int file_size(FILE* f);

muj_document load_file(char* filename)
{
	FILE* f = fopen(filename, "ro");
	size_t original_json_size = file_size(f);
	muj_compressed_json target = muj_allocate_compressed_json(original_json_size);
	muj_source source;
	source.file = f;

	muj_phase1(source, target);
	muj_document_table table = muj_allocate_document_table(target);
	muj_document document = muj_make_document(target, table);
	muj_phase2(document);
	fclose(f);
	return document;
}

void test_file(char* filename)
{
	printf("Testing %s...\n", filename);
	
	muj_document document = load_file(filename);
	
	muj_unload_document(document);
	
	if (muj_get_last_error() == 0)
		printf("Success.\n");
}

void test_doubles()
{
	char* filename = "../../test/doubles.json";
	printf("Testing doubles...\n");
	
	muj_document document = load_file(filename);
	
	size_t compressed_json_size = document.json.json_max_size + 1;
	char compressed_json[compressed_json_size]; compressed_json[compressed_json_size-1] = 0;
	memcpy(compressed_json, document.json.json_target, compressed_json_size);
	
	printf("CJSON: %s\n", compressed_json);
	
	MUJ_INDEX array = muj_get_root_object(document.table);
	
	size_t num_doubles = muj_array_count_number_of_elements(array, document);
	MUJ_INDEX all_doubles[num_doubles];
	muj_array_copy_elements(all_doubles, array, document);
	for( int i=0; i<num_doubles; i++)
	{
		double d = muj_get_double(all_doubles[i], document);
		printf("%f\n", d);
	}
	
	muj_unload_document(document);
}

void test()
{
	size_t numFiles = sizeof(files) / sizeof(char*);
	for( int i=0; i<numFiles; i++)
	{
		char* file = files[i];
		test_file(file);
	}
	test_doubles();	
}

int main()
{
	test();
	char c;
	scanf("%c", &c);
}