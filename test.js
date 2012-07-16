var server = http.createServer("127.0.0.1", 4000);

server.listen(function(req) {
	log(req);
});
