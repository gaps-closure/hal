==== libzmq =====

Source:

zeromq3_4.3.2.orig.tar.gz

Avalible from one of:
(http://deb.debian.org/debian/pool/main/z/zeromq3/zeromq3_4.3.2.orig.tar.gz)
(https://github.com/zeromq/libzmq/archive/v4.3.2.tar.gz)

Sha256sum
(02ecc88466ae38cf2c8d79f09cfd2675ba299a439680b64ade733e26a349edeb)


Build Notes:
Run (ensuring no errors) 
1) ./autogen.sh
2) ./configure --enable-static=yes --enable-shared=no --with-libsodium --without-libgssapi_krb5
3) make

The output static library is in "src/.libs/libzmq.a"

4) run 'strip -x libzmq.a' to reduce the size before checking in

