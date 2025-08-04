#!/usr/bin/node
const http = require('http');
const querystring = require('querystring');

// Set content type for HTTP response
console.log("Content-Type: text/html");
console.log("");

// HTML response
console.log("<html>");
console.log("<head><title>Node.js CGI Example</title></head>");
console.log("<body>");
console.log("<h1>Node.js CGI Script</h1>");

// Get request method
const method = process.env.REQUEST_METHOD || "GET";
console.log(`<p>Request Method: ${method}</p>`);

// Handle GET request
if (method === "GET") {
    const queryString = process.env.QUERY_STRING || "";
    console.log(`<p>Query String: ${queryString}</p>`);
    if (queryString) {
        const params = querystring.parse(queryString);
        for (const [key, value] of Object.entries(params)) {
            console.log(`<p>${key}: ${value}</p>`);
        }
    }
}

// Handle POST request
if (method === "POST") {
    const contentLength = parseInt(process.env.CONTENT_LENGTH || "0");
    if (contentLength > 0) {
        let postData = "";
        process.stdin.setEncoding('utf8');
        process.stdin.on('data', (chunk) => {
            postData += chunk;
        });
        process.stdin.on('end', () => {
            console.log(`<p>POST Data: ${postData}</p>`);
            const params = querystring.parse(postData);
            for (const [key, value] of Object.entries(params)) {
                console.log(`<p>${key}: ${value}</p>`);
            }
            console.log("</body>");
            console.log("</html>");
        });
    } else {
        console.log("<p>No POST data received.</p>");
        console.log("</body>");
        console.log("</html>");
    }
} else {
    console.log("</body>");
    console.log("</html>");
}