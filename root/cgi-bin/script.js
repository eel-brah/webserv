#!/usr/bin/env node

const { env, stdin, stdout, exit } = process;

const method = env.REQUEST_METHOD || "GET";
const queryString = env.QUERY_STRING || "";
const contentLength = parseInt(env.CONTENT_LENGTH || "0", 10);

function respond(body) {
  stdout.write("Content-Type: text/html\r\n");
  stdout.write("Status: 200 OK\r\n");
  stdout.write("\r\n");
  stdout.write("<html>\n");
  stdout.write("<body>\n");
  stdout.write("<h1>Hello, CGI!</h1>\n");
  stdout.write(`<p>Method: ${method}</p>\n`);
  stdout.write(`<p>Query String: ${queryString}</p>\n`);
  if (body) {
    stdout.write(`<p>POST Body: ${body}</p>\n`);
  }
  stdout.write("</body>\n");
  stdout.write("</html>\n");
  exit(0);
}

if (method === "POST" && contentLength > 0) {
  let body = "";
  stdin.setEncoding("utf8");

  stdin.on("data", chunk => {
    body += chunk;
    if (body.length >= contentLength) {
      respond(body);
    }
  });
} else {
  respond("");
}

