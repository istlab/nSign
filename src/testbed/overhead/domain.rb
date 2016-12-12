#!/usr/bin/env ruby
require 'rubygems'
require 'domainatrix'

unless ARGV.length == 1 
	puts "usage: domain.rb url"
end

url = Domainatrix.parse(ARGV[0])
print url.domain + "\n"

