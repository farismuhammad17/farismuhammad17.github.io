*This is the source code of my porfolio hosted at [farismuhd.dev](https://farismuhd.dev)*

The backend server, written in C++, is designed to make the subdomain system simpler. Note that the DNS records must be set up for each subdomain or a wildcard, such as `*.farismuhd.dev`, must be used. The backend server will handle the routing and management of these subdomains, allowing for easier deployment and maintenance. If the server is changed, use the provided `Makefile` to compile it.

# Server Routing System

The main server, hosted on (say) `farismuhd.dev`, will be found through a folder named `root`. Each subdomain is existent as a folder. For example: to host `xyz.farismuhd.dev`, a folder named `/xyz` can be made, within which must be an `index.*` file. HTML is preferred, but if you wish to route to any other file, you may, as long as it is named `index`.

The server routes CSS, JavaScript, and any other required files for each of the subdomains appropriately. Each site can also come with their own `robots.txt` and `sitemap.xml`.

# Error Handling

Any error codes (4XX, 5XX, etc.) are handled by the server by returning the content of an `error.html`, which has designated variables that the C++ server writes over when responding to the client.

---

*Copyright (c) 2026 Faris Muhammad (farismuhd.dev). All rights reserved.*
