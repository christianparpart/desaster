#! /usr/bin/env ruby

if File.exist? './config/environment'
  puts "Working directory does not seem to be a Rails environment."
  exit 1
end

# load Rails environment
require './config/environment'

class DesasterWrapper
  def initialize
    @master = IO.open(ENV["DESASTER_FD"].to_i, "rw", :binmode => true)
  end

  def run
    while job = get_job do
      process_job(job)
    end
  end

  private

  def get_job
    job = @master.gets
    job = Marshal.load(job)
    job
  rescue => e
    puts "Unmarshalling job failed: #{e}"
    nil
  end

  def process_job(job)
    puts "Processing: #{job.inspect}"
  end
end

puts "Environment loaded. Waiting for jobs now ..."
wrapper = DesasterWrapper.new
wrapper.run
