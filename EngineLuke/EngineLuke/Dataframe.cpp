#include "Dataframe.h"
#include "csvparser.h"
#include <cassert>
/////////////////////////////////////////////////////////////// 
// attribute

Attribute::Attribute(const std::string& content)
	: _content(content)
{
}

/////////////////////////////////////////////////////////////// 
// Instance

int Instance::GetAttributeCount() const
{
	return int(_attributes.size());
}

const Attribute& Instance::GetAttribute(size_t idx) const
{
	return _attributes[idx];
}

void Instance::AddAttribute(const std::string& attribute)
{
	_attributes.push_back(Attribute(attribute));
}

/////////////////////////////////////////////////////////////// 
// Dataframe

Dataframe::~Dataframe()
{
	for (auto instance : _instances)
		delete instance;
}
size_t Dataframe::GetAttributeCount() const
{
	return _attributeNames.size();
}


bool Dataframe::BuildFromCsv(const std::string& filename, bool hasHeader)
{
	// I am using open-source csv parser.
	CsvParser *csvParser = CsvParser_new(filename.c_str(), ",", int(hasHeader));

	CsvRow* header = CsvParser_getHeader(csvParser);
	if (!header)
	{
		_errormsg = CsvParser_getErrorMessage(csvParser);
		return false;
	}

	// build attribute types
	int attributeCount = CsvParser_getNumFields(header);
	const char **parsedHeader = CsvParser_getFields(header);
	for (int i = 0; i < attributeCount; ++i)
		AddAttribute(parsedHeader[i]);

	// build instances
	while (CsvRow* row = CsvParser_getRow(csvParser))
	{
		const char **parsedRow = CsvParser_getFields(row);
		Instance* instance = CreateInstance();
		for (int i = 0; i < attributeCount; ++i)
			instance->AddAttribute(parsedRow[i]);
	}

	_errormsg = "";

	CsvParser_destroy(csvParser);
	return true;
}

const std::string& Dataframe::GetAttributeName(size_t i)
{
	assert(i < GetAttributeCount());
	return _attributeNames[i];
}

Instance* Dataframe::CreateInstance()
{
	_instances.emplace_back(new Instance);
	return _instances.back();
}

void Dataframe::AddAttribute(const std::string& attributeName)
{
	_attributeNames.emplace_back(attributeName);
	_attributeMap.emplace(attributeName, (unsigned)_attributeNames.size() - 1);
}

const Attribute& Dataframe::GetInstanceAttribute(
	const Instance* instance, const std::string& attName)
{
	return instance->GetAttribute(int(_attributeMap[attName]));
}

size_t Dataframe::GetInstanceCount() const
{
	return int(_instances.size());
}

const Instance& Dataframe::GetInstance(size_t idx)
{
	return *_instances[idx];
}

std::string Dataframe::GetErrorMessage() const
{
	return _errormsg;
}

bool Dataframe::Merge(Dataframe& rhs)
{
	if (_attributeNames != rhs._attributeNames)
		return false;

	_instances.insert(
		_instances.end(),
		std::make_move_iterator(rhs._instances.begin()),
		std::make_move_iterator(rhs._instances.end()));
	rhs._instances.clear();
	return true;
	
}

void Dataframe::ToCsv(std::ostream& o)
{
	for (size_t i = 0; i < _attributeNames.size(); ++i)
	{
		if (i > 0)
			o << ",";
		o << _attributeNames[i];
	}
	o << std::endl;

	for (auto instance : _instances)
	{
		for (size_t i = 0; i < _attributeNames.size(); ++i)
		{
			if (i > 0)
				o << ",";
			o << instance->GetAttribute(i).AsString();
		}
		o << std::endl;
	}
}