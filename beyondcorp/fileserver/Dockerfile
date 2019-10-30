FROM golang:1.13 AS builder

COPY main.go main.go

RUN CGO_ENABLED=0 go build -o /app main.go

FROM scratch

COPY --from=builder /app /app
COPY content /content

CMD [ "/app" ]
