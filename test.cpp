#include <mujson.hpp>
#include <fstream>
#include <iostream>
#include <cassert>

int main(int argc, char **argv)
{
	std::ifstream inFile("../../test/regular.json", std::ios::binary);
	assert(inFile.good());
	Json::Reader reader;
	Json::Value root;
	bool success = reader.parse(inFile, root);
	if (!success)
		std::cout << "Parse failed." << std::endl;
	
	if (root.isArray())
	{
		std::cout << "Root is array." << std::endl;
		
		Json::Array rootArray(root);
		
		for( unsigned i=0; i<rootArray.size(); i++)
		{
			Json::Value val = rootArray[i];
			if (val.isObject())
			{
				std::cout << "Element: " << val["name"].asString() << std::endl;
				Json::Object valObject(val);
				for( unsigned j=0; j < valObject.size(); j++)
				{
					Json::KeyValuePair kvp = valObject[j];
					std::cout << kvp.key << " : " << (kvp.value.isString()?kvp.value.asString():"") << std::endl;
				}
			}
		}
	}
	else
	{
		std::cout << "Root is not array." << std::endl;
	}
	
	std::cin.ignore();
	
	return 0;
}
