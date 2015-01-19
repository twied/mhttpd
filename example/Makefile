all: echoserver fileserver

echoserver:
	@$(MAKE) -C echoserver

fileserver:
	@$(MAKE) -C fileserver

clean:
	@$(MAKE) -s -C echoserver clean
	@$(MAKE) -s -C fileserver clean

.PHONY: all echoserver fileserver clean
