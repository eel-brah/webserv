#!/usr/bin/perl
use strict;
use warnings;

my $method = $ENV{'REQUEST_METHOD'} || 'GET';
my $query_string = $ENV{'QUERY_STRING'} || '';
my $content_length = $ENV{'CONTENT_LENGTH'} || 0;
my $body = '';

if ($method eq 'POST' && $content_length > 0) {
    read(STDIN, $body, $content_length);
}

print "content-type: text/html\r\n";
print "status: 200 OK\r\n";
print "\r\n";

print "<html>\n";
print "<body>\n";
print "<h1>Hello from perl CGI!</h1>\n";
print "<p>Method: $method</p>\n";
print "<p>Query String: $query_string</p>\n";
if ($body) {
    print "<p>POST Body: $body</p>\n";
}
print "</body>\n";
print "</html>\n";
