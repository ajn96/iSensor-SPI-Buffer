from spi_buf_cli import ISensorSPIBuffer

buf = ISensorSPIBuffer("COM11")
print(buf.version())
