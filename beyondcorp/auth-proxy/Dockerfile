FROM golang:1.13 AS builder

COPY . /app

RUN cd /app && CGO_ENABLED=0 go build -o /app/app .

FROM scratch

COPY --from=builder /app/app /app

CMD [ "/app" ]
