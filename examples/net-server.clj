
; server 
(future (let [
    listen (net/listen (net/bind (net/socket) "127.0.0.1" 1234) 1)
    client (net/accept listen)
    ]
    (net/send client "Hello from server!")
    (println (net/recv client))
))

; sleep (wait for server to init)
(Thread/sleep 2000) 

; client
(future (let [
    conn (net/connect (net/socket) "127.0.0.1" 1234)
    ]
    (net/send conn "Hello from client!")
    (println (net/recv conn))
))
