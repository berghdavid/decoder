FROM alpine as build-env
RUN apk add --no-cache build-base && apk --no-cache add curl-dev
WORKDIR /app
COPY . .
# Compile the binaries
RUN make server

FROM alpine
RUN apk --no-cache add curl-dev
COPY --from=build-env /app/server /app/server
WORKDIR /app
CMD ["/app/server $HOST $PORT $PENDING $MAX_BUF $REUSE $FORWARD"]
