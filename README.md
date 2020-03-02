# HighLoad-Server
Задание:
1. Разработать веб-сервер для отдачи статики с диска. Язык программирования и технологию многопоточной обработки соединений выбрать самостоятельно. Разрешается использовать библиотеки помогающие реализовать асинхронную обработку соединений (libevent/libev и им подобные), запрещается использовать библиотеки реализующие мультипоточную обработку или какую-либо часть обработки HTTP. Запрещено использование языков Go и Node.js по причине встроенной многопоточности.
2. Провести нагрузочное тестирование, проверку стабильности и корректности работы.

3. Требования и методика тестирования по ссылке: https://github.com/init/http-test-suite

Web server test suite
=====================

Implement a Web server in the programming language of your choice. Libraries for helping manage TCP socket connections *may* be used (libevent allowed). Libraries that implement any part of HTTP or multiprocessing model *must not* be used. Languages and platforms implementing hidden multithreading (Go, Node.js) are not allowed.

## Requirements ##

* Respond to `GET` with status code in `{200,404,403}`
* Respond to `HEAD` with status code in `{200,404,403}`
* Respond to all other request methods with status code `405`
* Directory index file name `index.html`
* Respond to requests for `/<file>.html` with the contents of `DOCUMENT_ROOT/<file>.html`
* Requests for `/<directory>/` should be interpreted as requests for `DOCUMENT_ROOT/<directory>/index.html`
* Respond with the following header fields for all requests:
  * `Server`
  * `Date`
  * `Connection`
* Respond with the following additional header fields for all `200` responses to `GET` and `HEAD` requests:
  * `Content-Length`
  * `Content-Type`
* Respond with correct `Content-Type` for `.html, .css, js, jpg, .jpeg, .png, .gif, .swf`
* Respond to percent-encoding URLs
* Correctly serve a 2GB+ files
* No security vulnerabilities

## Testing environment ##

* Put `Dockerfile` to web server repository root
* Prepare docker container to run tests:
  * Read config file `/etc/httpd.conf`
  * Expose port 80

Config file spec:
```
cpu_limit 4       # maximum CPU count to use (for non-blocking servers)
thread_limit 256  # maximum simultaneous connections (for blocking servers)
document_root /var/www/html
```

Run tests:
```
git clone https://github.com/init/http-test-suite.git
cd http-test-suite

docker build -t bykov-httpd https://github.com/init/httpd.git
docker run -p 80:80 -v /etc/httpd.conf:/etc/httpd.conf:ro -v /var/www/html:/var/www/html:ro --name bykov-httpd -t bykov-httpd

./httptest.py
```

## Installation ##
### With Docker
```sh
git clone https://github.com/moguchev/HighLoad-Server.git
cd HighLoad-Server
docker build . -t server
docker run -p 80:80 -t server --name server
```
### With CMake
```sh
git clone https://github.com/moguchev/HighLoad-Server.git
cd HighLoad-Server
cmake .
make
./Server
```
### Note
Do not forget write your configurations in *httpd.conf* file.
