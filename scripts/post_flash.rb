require 'serialport'

def send_command(port, command)
  serial_port = SerialPort.new(port, 115200, 8, 1, SerialPort::NONE)
  serial_port.write(command)
  serial_port.write("\n")
  serial_port.close
end

if __FILE__ == $0
  port = ARGV[0]
  command = ENV['POST_FLASH_CMD']
  if port.nil? || command.nil?
    puts "Nothing to do"
    exit
  end
  puts "Sending command: #{command} to port: #{port}"
  send_command(port, command)
end