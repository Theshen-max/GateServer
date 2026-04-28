#ifndef JSONMGR_H
#define JSONMGR_H

#include "json/json.h"
#include "json/value.h"
#include "json/reader.h"
#include <string>
#include <memory>

class JsonMgr
{
public:
	JsonMgr(const JsonMgr&) = delete;

	JsonMgr& operator=(const JsonMgr&) = delete;

	static std::string jsonToString(const Json::Value& json, bool format = true);

	static Json::Value stringToJson(const std::string& str);

private:
	JsonMgr() = default;
};


#endif //JSONMGR_H
