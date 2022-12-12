(def url (nth *command-line-args* 2))

(println (slurp url))
; or (http/get url)
