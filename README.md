# simple-web-server

### Dku 32194394 Cho Junghee

simple web server with c language

use socket.h

pages
- index : main page
- about : check list for this server
- post : test post method
- result : result of post method (use Query String)
- 404 : for not implemented pages

if request arrived at server, make thread (request handler)

the request handler parse the input and print out the data according to the HTTP GET request.
and make some HTTP header and body, send it client
