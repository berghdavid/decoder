FROM alpine as build-env
RUN apk add --no-cache build-base && apk --no-cache add curl-dev
WORKDIR /app
COPY . .
RUN make server

FROM alpine
RUN apk --no-cache add curl-dev
COPY --from=build-env /app/server /app/server
WORKDIR /app
EXPOSE 5124
CMD /app/server $HOST $PORT $PENDING $MAX_BUF $REUSE $FORWARD
