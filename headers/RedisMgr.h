#ifndef REDISMGR_H
#define REDISMGR_H

#include <sw/redis++/redis++.h>

class RedisMgr
{
enum RedisDB
{
	Default = 0,
	UserInfo = 1
};

public:
	static sw::redis::Redis& getRedis();

	static sw::redis::Redis& getUserInfoRedis();

	RedisMgr(const RedisMgr&) = delete;

	RedisMgr& operator=(const RedisMgr&) = delete;

private:
	RedisMgr() = default;

	static sw::redis::ConnectionOptions initConnectionOptions(RedisDB db = Default);

	static sw::redis::ConnectionPoolOptions initPoolOptions();

};


#endif //REDISMGR_H
