git clone --depth=1 https://github.com/olajep/riscv-nginx.git

cd riscv-nginx

./auto/configure \
  --without-http_rewrite_module \
  --without-http_gzip_module \
  --with-cc-opt="-DAO_USE_PTHREAD_DEFS=1 -DNGX_HAVE_MAP_ANON=1 -DNGX_HAVE_LIBATOMIC=1 -DNGX_SYS_NERR=150 -Isrc/libatomic_ops" \
  --with-libatomic \
  --with-ld-opt="-lpthread"

make -j$(nproc)
sudo make install
