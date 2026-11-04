FROM alpine:latest

RUN apk add git cmake build-base pkgconf libmodbus-dev yaml-cpp-dev linux-headers

COPY . .

RUN cmake -S . -B build
RUN cmake --build build
RUN install ./build/faux /bin

CMD [ "faux", "serve", "./maps/sample1.yaml" ]
