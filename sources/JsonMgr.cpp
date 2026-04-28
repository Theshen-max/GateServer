#include "../headers/JsonMgr.h"

std::string JsonMgr::jsonToString(const Json::Value& json, bool format)
{
	Json::StreamWriterBuilder _writer_builder;
	if (!format)
	{
		_writer_builder["indentation"] = "";
	}
	return Json::writeString(_writer_builder, json);
}

Json::Value JsonMgr::stringToJson(const std::string& str)
{
	Json::CharReaderBuilder _reader_builder;
	std::string errors;
	Json::Value root;
	std::unique_ptr<Json::CharReader> reader(_reader_builder.newCharReader());
	bool parse_ok = reader->parse(str.c_str(), str.c_str() + str.size(), &root, &errors);
	if (!parse_ok)
	{
		root.clear();
		root["parse_error"] = errors;
	}
	return root;
}
