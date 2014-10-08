all:
	@cc -g -w s_client.c -o s_client -lssl -lcrypto
	@cc -g -w memcached.c ssl_helper.c connection.c -o memcached -lpthread -lssl -lcrypto
	@cc -g -w s_test_client.c -o s_test_client -lssl

.PHONY: clean
clean:
	@rm -rf memcached
	@rm -rf s_client
	@rm -rf test_client



