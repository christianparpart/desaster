#! /usr/bin/env ruby

if File.exist? './config/environment'
  puts "Working directory does not seem to be a Rails environment."
  exit 1
end

# load Rails environment
require './config/environment'

puts "Environment loaded. Waiting for jobs now ..."
while job = gets do
  begin
    job = Marshal.load(job)
  rescue => e
    puts "Unmarshalling job failed: #{e}"
    next
  end

  puts job.inspect
end
