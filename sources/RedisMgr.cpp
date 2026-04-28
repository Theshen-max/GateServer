//
// Created by 27044 on 26-3-10.
//
#include "../headers/ConfigMgr.h"
#include "../headers/RedisMgr.h"

sw::redis::Redis& RedisMgr::getRedis()
{
	static sw::redis::Redis redis(initConnectionOptions(Default), initPoolOptions());
	return redis;
}

sw::redis::Redis& RedisMgr::getUserInfoRedis()
{
	static sw::redis::Redis redis(initConnectionOptions(UserInfo), initPoolOptions());
	return redis;
}

sw::redis::ConnectionOptions RedisMgr::initConnectionOptions(RedisDB db)
{
	sw::redis::ConnectionOptions opts;

	opts.host = ConfigMgr::getInstance()["Redis"]["Host"];
	opts.port = std::stoi(ConfigMgr::getInstance()["Redis"]["Port"]);
	opts.password = ConfigMgr::getInstance()["Redis"]["Passwd"];
	opts.socket_timeout = std::chrono::milliseconds(200);	// 网络IO超时机制， 200ms
	opts.db = db;

	return opts;
}

sw::redis::ConnectionPoolOptions RedisMgr::initPoolOptions()
{
	sw::redis::ConnectionPoolOptions opts;
	opts.size = 10;
	opts.wait_timeout = std::chrono::milliseconds(100); // 单次等待最大限额，避免长时间等待空闲连接
	return opts;
}





