mhttpd
======
mhttpd is a Micro HTTP Daemon. Similar to Apache Tomcat's Servelets it provides a minimal environment to interact with http clients.

It is meant for small and tiny setups (hence the /micro/ in the name) where a full blown apache / nginx would be overkill.

Getting started
---------------
Build mhttpd as a library:
```sh
./autogen.sh
make && make install
```

Build the examples:
```sh
cd example && make
```

Now start the fileserver:
```sh
cd fileserver
./fileserver .
```

You can now point your browser to `http://localhost:8080/` and watch the fileserver do its work.

License
-------
(c) 2015 Tim Wiederhake, licensed under New BSD.
