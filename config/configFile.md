# Server
1. Define all the interface:port pairs on which your server will
	listen to (defining multiple websites served by your program).
	# listen = 0.0.0.0:8080;

2. set up default error pages.
	# error_page 404 = /errors/404.html;

3. Set the maximum allowd size of request-body.
	===> client_max_bdoy_size = 2M;

4. Specify rules or configurations on a URL/route (no regex required here), for a
	website, among the following:
	# location {
	
	}