#include <mujson.hpp>

#include <iostream>

extern "C"
{

#include <mujson.c>

bool muj_read_byte(muj_source source, char* byte)
{
	int c = source.file->get();
	if (c == std::char_traits<char>::eof())
		return false;
	*byte = (char)c;
	return true;
}

bool muj_peek_byte(muj_source source, char* byte)
{
	int peekResult = source.file->peek();
	if (peekResult == std::char_traits<char>::eof())
		return false;
	*byte = (char)peekResult;
	return true;
}

}

namespace MUJSON_NAMESPACE
{
	
Reader::Reader()
{
	memset(&document, 0, sizeof(document));
}

Reader::~Reader()
{
	muj_free_compressed_json(document.json);
	muj_free_document_table(document.table);
}

bool Reader::parse(std::istream& inStream, Value& root)
{
	std::streampos position = inStream.tellg();
	inStream.seekg(0, std::ios_base::end);
	size_t originalJsonSize = inStream.tellg();
	inStream.seekg(position);
	
	std::cout << "Json size: " << originalJsonSize << std::endl;
	
	document.json = muj_allocate_compressed_json(originalJsonSize);
	
	muj_source source;
	source.file = &inStream;
	
	muj_phase1(source, document.json);
	
	document.table = muj_allocate_document_table(document.json);
	
	if (muj_get_last_error())
		return false;
		
	muj_phase2(document);
	
	root = Value(*this, 0);
	
	return !(muj_get_last_error());
}

Value Value::operator[](const char* name) const
{
	return Value( *document, muj_find_value_of_key_in_object(index, const_cast<char*>(name), document->getDocument()));
}

Value Value::operator[](size_t atIndex) const
{
	return Value( *document, muj_get_element_from_array(index, atIndex, document->getDocument()));
}

float Value::asFloat() const
{
	return asDouble();
}
	
double Value::asDouble() const
{
	MUJSON_ASSERT(isNumeric());
	return muj_get_double(index, document->getDocument());
}
	
bool Value::asBool() const
{
	MUJSON_ASSERT(isBoolean());
	return muj_is_true(index, document->getDocument());
}
	
int Value::asInt() const
{
	return asLong();
}
	
long Value::asLong() const
{
	MUJSON_ASSERT(isNumeric());
	return muj_get_long(index, document->getDocument());
}
	
std::string Value::asString() const
{
	std::string out;
	out.resize(muj_get_string_length_excluding_null(index, document->getDocument()));
	muj_copy_string(&out[0], index, document->getDocument());
	return out;
}

bool Value::isArray() const
{
	return muj_is_array(index, document->getDocument());
}
	
bool Value::isObject() const
{
	return muj_is_object(index, document->getDocument());
}
	
bool Value::isString() const
{
	return muj_is_string(index, document->getDocument());
}
	
bool Value::isNumeric() const
{
	return muj_is_number(index, document->getDocument());
}
	
bool Value::isConstant() const
{
	return muj_is_constant(index, document->getDocument());
}
	
bool Value::isBoolean() const
{
	return muj_is_boolean(index, document->getDocument());
}
	
bool Value::isTrue() const
{
	MUJSON_ASSERT(isBoolean());
	return muj_is_true(index, document->getDocument());
}
	
bool Value::isFalse() const
{
	MUJSON_ASSERT(isBoolean());
	return muj_is_false(index, document->getDocument());
}
	
bool Value::isNull() const
{
	return muj_is_null(index, document->getDocument());
}

bool Value::empty() const
{
	if (isArray())
		return muj_is_array_empty(index, document->getDocument());
	if (isObject())
		return muj_is_object_empty(index, document->getDocument());
	return false;
}

Object::Object(const Value& value)
	: index(value.index)
	, document(value.document)
{
	MUJSON_ASSERT(value.isObject());
	if (!empty())
	{
		size_t size = muj_object_count_number_of_children(index, document->getDocument());
		keyValuePairs.resize(size); // TODO: This currently also copies the key indices, which we don't need because we store the strings seperately
		muj_object_copy_children(&keyValuePairs[0], index, document->getDocument());
		keys.resize(size);
		for( unsigned i=0; i<keyValuePairs.size(); i++)
		{
			MUJ_INDEX key = keyValuePairs[i].key;
			keys[i].resize(muj_get_string_length_excluding_null(key, document->getDocument()));
			muj_copy_string(&keys[i][0], key, document->getDocument());
		}
	}
}
	
Value Object::operator[](const char* name) const
{
	for( unsigned i=0; i<keys.size(); i++)
	{
		if (keys[i] == name)
			return Value(*document, keyValuePairs[i].value);
	}
	return Value(*document, 0);
}

KeyValuePair Object::operator[](int index) const
{
	KeyValuePair out;
	out.key = keys[index];
	out.value = Value(*document, keyValuePairs[index].value);
	return out;
}

bool Object::empty() const
{
	return muj_is_object_empty(index, document->getDocument());
}

Array::Array(const Value& value)
	: index(value.index)
	, document(value.document)
{
	MUJSON_ASSERT(value.isArray());
	if (!empty())
	{
		size_t size = muj_array_count_number_of_elements(index, document->getDocument());
		elements.resize(size);
		muj_array_copy_elements(&elements[0], index, document->getDocument());
	}
}
	
Value Array::operator[](size_t index) const
{
	return Value(*document, elements[index]);
}

bool Array::empty() const
{
	return muj_is_array_empty(index, document->getDocument());
}
	
}