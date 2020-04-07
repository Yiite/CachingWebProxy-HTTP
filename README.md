# Caching Web Proxy (HW2 for ECE 568 ERSS 2020 Spring)

By Wenge Xie, Yiteng Lu

## Set Up

To run the program, go to ```erss-hwk2-wx50-yl561/``` folder, then run

```
sudo docker-compose up
```
You don't need to create a ```./logs``` directory by hand. Our code will create and mount it to the docker container for you. After using ```docker-compose up``` to run our program, you can check the ```proxy.log``` file inside the ```./logs``` directory automatically created.

Note that our proxy is set to be running on the port 12345 on the local machine. It means that you can connect to the proxy by using your ```local_ip:12345```.

## Set up

Our proxy is based on socket programming. It will build 2 connections ---- with the client (browser, terminal, etc.) and the origin server (Google.com, Youtube.com, etc.) Our proxy is divide into two parts. The "server" part is accepting the requests from the browers, and the "client" part is connected to the origin server for sending packages from the browsers. For the "server" part, the ```socket()```, ```bind()```, ```accecpt()```, ```recv()``` are called to bind a socket to the port 12345 for listening incoming requests. For the "client" part, the ```socket()```, ```connect()``` and ```send()``` functions are called for build up a connection with the origin server and send the packages from it. When receiving the response, first, the "client" part will call the ```recv()``` to rececive the response from the origin server, and the "server" part will call ```send()``` function to send the response back to the browser.

## Process the method of GET

When the browser send us a GET request, our proxy will read the head fields from the request (which is the full request). The proxy will send the request non-changed to the origin server, and pass the response from the server to the client. 

## Process the method of CONNECT

When the brower send the proxy a CONNECT request, the proxy will first establish the connection with the origin server by using the ```socket()``` function. Then the proxy will send a "200 Connection established" response to the client. After that, the proxy will pass the packges from the client the the origin server and the packages from the client blindly.

## Process the method of POST
When the browser send the proxy a POST request, the proxy will first read the header from the request, and parse that head for the content length information. The content-length information is used for receiving the body of that request. After received the whole body of the request, the proxy will pass the request to the origin server, and send the response back to the browser.

## Receive function for response
Two metrics are used for judging the length of the response length when receiving the response. The proxy will first check if the response has one header field of "Transfer-Encoding". If it finds this field and see "Chunked" as the value, the proxy will allocate a big space and receive the body once. If the proxy didn't see the "Transfer-Encoding" field, it will try to look for the "Content-Length" field, and use that information for receiving the body of the response.

## Cache implementation

The proxy is using the LRU algorithm to implement a cache. The size of the cache is set to be 128. New cache nodes will be placing at the front of the cache list and the cache node which has been used will be move to the head of the list. If the cache list is longer than 128, the node at the back will be deleted for freeing more space for the new nodes. Sever header fields are checked to determine if the proxy will cache the response or not: "Cache-Control, Expires, Last-Modified". The lifetime of the cached response is computed by using the information from the "Cache-Control, Expires, Last-Modified, Date" fields. Heuristic lifetime is set to be 10% of "Date - Last-Modified". The current age of the response is also computed using the RFC rules. A response is determined to be "stale" if its age exceeded the lifetime. A revalidation funtion is implemented for refreshing the cache. If the cache sees some fields (like no-cache, max-age=0) or determined that the current cached page is stale, it will call the function to validate if the response need to be refreshed or not by using some validators like E-tags and Last-Modified. If it was returned with a "304 not modified" response, it will keep the cached response. Otherwise, it will refresh the response. Same request is sent to the origin server and new response will replace the old ones. The "Vary" field will also affect the cache function. The cache will compare the values in the fields mentioned in the "Vary" field between the origin request and the current request. If they are different, the cache will revalidate the page immediately. Stale pages may be returned to the browser if it is allowed by the RFC rules with a startline "110 Stale Response".

## Handling unusual cases

The proxy will check the format both for the request and the response. If the request if malformated, the proxy will return the browser with a "400 Bad Request" response. If the response is malformated, the proxy will return a "502 Bad GateWay".

