#include <exception>
//Well, my first attempt with exceptions
//May be very unstable but let's have fun
//this code is garbage anyway
class SMBShareNotFoundException: public exception
{
  virtual const char* what() const throw()
  {
    return "Share Not Found Exception";
  }
} smbsnfexception;

class ParamNotFoundException: public exception
{
  virtual const char* what() const throw()
  {
    return "Parameter Not Found Exception";
  }
} pnfexception;
