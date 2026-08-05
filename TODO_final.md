* *TODO*

* <Strategy>
	1. Remove unneccessary print statements. Leave Request, Parsed_Request, location, modifiedURI, Response prints.
	2. check tests/ -> if failed clarify why
	3. cleanup
	4. configFileParser
	6. Unallowed functions (realpath, timefunction) isFileAccessible()
	7. Copy constructer & assignment operator overloads.
	9. ConfigFileParser
		- should throw Error if error occurs?
		- filename validation?

* <ToDo list>
	- [] formSubmit() appendToFile()
	- [] config file -> ".conf" check
	- [X] cgi env setting correct? SCRIPT_NAME, PATH_INFO, PATH_TRANSLATED
	- [] Copy & assignment operators
	- [X] server max_body_size (location falls back to server-level if not set)
	- [] setting good limits (limits_defines.hpp)

* TestCases from /tests/
	* GET /../etc/passwd

* <print statement>
	- [X] config
	- [X] https
	- [X] methods
	- [X] parsers
