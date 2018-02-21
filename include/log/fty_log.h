/*  =========================================================================
    fty_log - Log management

    Copyright (C) 2014 - 2018 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
 */

#ifndef FTY_LOG_H_INCLUDED
#define FTY_LOG_H_INCLUDED

// Trick to avoid conflict with CXXTOOLS logger, currently the BIOS code
// prefers OUR logger macros
#if defined(LOG_CXXTOOLS_H) || defined(CXXTOOLS_LOG_CXXTOOLS_H)
#undef log_error
#undef log_debug
#undef log_info
#undef log_fatal
#undef log_warn
#else
#define LOG_CXXTOOLS_H
#define CXXTOOLS_LOG_CXXTOOLS_H
#endif

#ifdef __cplusplus
#include <log4cplus/configurator.h>
#endif
//Macro for logging

#ifdef __cplusplus
#define log_macro(level,ftylogger, ...) \
    do { \
        ftylogger->insertLog((level), __FILE__, __LINE__, __func__, __VA_ARGS__); \
    } while(0)
#else
#define log_macro(level,ftylogger, ...) \
    do { \
        insertLog(ftylogger,(level), __FILE__, __LINE__, __func__, __VA_ARGS__); \
    } while(0)
#endif

/* Prints message with TRACE level. 0 <=> log4cplus::TRACE_LOG_LEVEL */
#define log_trace(ftylogger,...) \
        log_macro(0,ftylogger, __VA_ARGS__)

/* Prints message with DEBUG level. 10000 <=> log4cplus::DEBUG_LOG_LEVEL */
#define log_debug(ftylogger,...) \
        log_macro(10000,ftylogger, __VA_ARGS__)

/* Prints message with INFO level. 20000 <=> log4cplus::INFO_LOG_LEVEL */
#define log_info(ftylogger,...) \
        log_macro(20000,ftylogger, __VA_ARGS__)

/* Prints message with WARNING level 30000 <=> log4cplus::WARN_LOG_LEVEL*/
#define log_warning(ftylogger,...) \
        log_macro(30000,ftylogger, __VA_ARGS__)

/* Prints message with ERROR level 40000 <=> log4cplus::ERROR_LOG_LEVEL*/
#define log_error(ftylogger,...) \
        log_macro(40000,ftylogger, __VA_ARGS__)

/* Prints message with FATAL level. 50000 <=> log4cplus::FATAL_LOG_LEVEL*/
#define log_fatal(ftylogger,...) \
        log_macro(50000,ftylogger, __VA_ARGS__)

#define LOGPATTERN "%d{%b %-2d %H:%M:%S.%q} %h %t [%i] -%-5p- %M (%l) %m%n"

//  @interface
#ifdef __cplusplus

class Ftylog
{
private:
  std::string _agentName;
  std::string _configFile;
  std::string _layoutPattern;
  log4cplus::Logger _logger;
  log4cplus::ConfigureAndWatchThread * _watchConfigFile;

  //Return true if level is included in the logger level
  bool isLogLevel(log4cplus::LogLevel level);

  //Set the console appender
  void setConsoleAppender();

  //Set log level with level from syslog.h for debug, info, warning, error and fatal
  // log level set to trace level otherwise
  void setLogLevelFromEnv(const char* level);

  //Set needed variables from env 
  void setVarEnv();

  //Load appenders from the config file 
  // or set the default console appender if no can't load from the config file
  void loadAppenders();

public:
  //Constructor/destructor
  Ftylog(std::string _component, std::string logConfigFile = "");
  ~Ftylog();

  //getter
  std::string getAgentName();

  //setter
  //Set the path to the log config file
  //And try to load it
  void setConfigFile(std::string file);

  //Set the logger to a specific log level
  void setLogLevelTrace();
  void setLogLevelDebug();
  void setLogLevelInfo();
  void setLogLevelWarning();
  void setLogLevelError();
  void setLogLevelFatal();

  //Check the log level 
  bool isLogTrace();
  bool isLogDebug();
  bool isLogInfo();
  bool isLogWarning();
  bool isLogError();
  bool isLogFatal();

  /*! \brief do logging
    An internal logging function, use specific log_error, log_debug  macros!
    \param level - level for message, see \ref log4cplus::logLevel 
    \param file - name of file issued print, usually content of __FILE__ macro
    \param line - number of line, usually content of __LINE__ macro
    \param func - name of function issued log, usually content of __func__ macro
    \param format - printf-like format string
   */
  void insertLog(log4cplus::LogLevel level, const char* file, int line,
                 const char* func, const char* format, ...);

  //Load a specific appender if verbose mode is set to true : 
  // -Save the logger log level and set it to TRACE log level
  // -Remove an already existing ConsoleAppender
  // -For the other appender, set the threshold parameter to the old logger log level
  //    if no loglevel defined for this appender
  // -Add a new console appender
  void setVeboseMode();
};

#else
typedef struct Ftylog Ftylog;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

///////////////////////////////  
//wrapper to use for C code only
///////////////////////////////
//Constructor
Ftylog * new_ftylog(const char * component, const char * logConfigFile);
//destructor
void delete_ftylog(Ftylog * log);

//setter
void setConfigFile(Ftylog * log, const char * file);

//Set the logger to a specific log level
void setLogLevelTrace(Ftylog * log);
void setLogLevelDebug(Ftylog * log);
void setLogLevelInfo(Ftylog * log);
void setLogLevelWarning(Ftylog * log);
void setLogLevelError(Ftylog * log);
void setLogLevelFatal(Ftylog * log);

//Check the log level 
bool isLogTrace(Ftylog * log);
bool isLogDebug(Ftylog * log);
bool isLogInfo(Ftylog * log);
bool isLogWarning(Ftylog * log);
bool isLogError(Ftylog * log);
bool isLogFatal(Ftylog * log);

void insertLog(Ftylog * log, int level, const char* file, int line,
               const char* func, const char* format, ...);

void setVeboseMode(Ftylog * log);

#ifdef __cplusplus
}
#endif

//  Self test of this class
FTY_COMMON_EXPORT void
log_fty_log_test(bool verbose);

//  @end
#endif
