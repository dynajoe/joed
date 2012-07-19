var server = http.createServer("127.0.0.1", 4005);

server.listen(function(req) 
{

	var body = "<html><head><title>Your Request!</title></head><body><pre>" + req + "</pre></body></html>";

	response = "HTTP/1.1 200 OK\r\n" +
		"Content-Type: text/html; charset=UTF-8\r\n" +
		"Content-Length: " + body.length + "\r\n" +
		"\r\n" + body + "\n";

 	return response;
});
