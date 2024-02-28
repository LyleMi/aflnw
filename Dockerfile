FROM aflplusplus/aflplusplus

COPY . /aflnw
WORKDIR /aflnw

RUN export CC=/AFLplusplus/afl-clang-fast && \
    mkdir build && cd build && \
    cmake .. && make

