#include <cstdio>
#include <streambuf>
#include <string>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/progress.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include <curl/curl.h>

#include <json_spirit/json_spirit.h>

#include "object_recognition/db/couch.hpp"

namespace object_recognition
{
namespace curl
{
  struct writer
  {
    std::ostream& stream;
    writer(std::ostream& stream)
        :
          stream(stream)
    {
    }

    static size_t
    cb(char *ptr, size_t size, size_t nmemb, void *userdata)
    {
      if (!userdata)
      {
        return 0;
      }
      writer* data = static_cast<writer*>(userdata);
      data->stream.write(ptr, size * nmemb);
      return size * nmemb;
    }

    // size_t written;

  };

  struct reader
  {
    const std::istream& stream;

    reader(const std::istream& stream)
        :
          stream(stream)
    {
    }

    /**
     * static callback for c style void pointer cookies.
     */
    static size_t
    cb(char *ptr, size_t size, size_t nmemb, void *thiz)
    {
      if (!thiz)
      {
        return 0;
      }
      reader* data = static_cast<reader*>(thiz);
      return data->stream.rdbuf()->sgetn(ptr, size * nmemb);
    }
  };

  struct cURL: boost::noncopyable
  {
    enum HTTP_CODES
    {
      Continue = 100, OK = 200, Created = 201, Accepted = 202, BadRequest = 400
    };
    cURL()
        :
          curl_(curl_easy_init()),
          headers_(0),
          header_writer_(header_writer_stream_)
    {
      if (curl_ == 0)
        throw std::runtime_error("Unable to connect CURL.");
      reset();
    }

    ~cURL()
    {
      curl_slist_free_all(headers_);
      curl_easy_cleanup(curl_);
    }

    void
    perform()
    {
      //need to reset stream.
      header_writer_stream_.str("");
      curl_easy_perform(curl_);
      parse_response_header();
    }

    void
    setURL(const std::string& url)
    {
      curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    }

    void
    setHeader(const std::string& header)
    {
      headers_ = curl_slist_append(headers_, header.c_str());
      curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    }

    void
    setCustomRequest(const char* request_type)
    {
      curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, request_type);
    }
    void
    setWriter(writer* w)
    {
      curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &writer::cb);
      curl_easy_setopt(curl_, CURLOPT_WRITEDATA, w);
    }
    void
    setReader(reader* r)
    {
      curl_easy_setopt(curl_, CURLOPT_READFUNCTION, &reader::cb);
      curl_easy_setopt(curl_, CURLOPT_READDATA, r);
      curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);
    }
    void
    PUT()
    {
      curl_easy_setopt(curl_, CURLOPT_PUT, 1L);
    }

    void
    POST()
    {
      curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    }

    void
    GET()
    {
      curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    }

    void
    HEAD()
    {
      curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "HEAD");
    }
    void
    DELETE()
    {
      curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    void
    reset()
    {
      curl_slist_free_all(headers_);
      curl_easy_reset(curl_);
      headers_ = 0;
      curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, &writer::cb);
      curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_writer_);
    }

    int
    get_response_code() const
    {
      return response_status_code_;
    }
    const std::string&
    get_response_reason_phrase() const
    {
      return response_reason_phrase_;
    }

    const std::string&
    get_response_header(const std::string& header_name) const
    {
      if (header_response_values.count(header_name))
        return header_response_values.find(header_name)->second;
      else
        throw std::runtime_error(header_name + " does not exist");
    }

  private:
    void
    parse_response_header()
    {
      //parse codes
      std::string _, reason, x;
      do
      {
        header_writer_stream_ >> _ /*used to eatup the http version.*/
        >> response_status_code_;
        std::getline(header_writer_stream_, response_reason_phrase_, '\n');
        if (!response_reason_phrase_.empty())
          response_reason_phrase_.resize(response_reason_phrase_.size() - 1);
      } while (response_status_code_ == Continue); //handle continuecontinue

      header_response_values.clear();
      while (true)
      {
        std::string headerX, line;
        std::getline(header_writer_stream_, headerX, ':'); //eats the colon
        if (header_writer_stream_.eof())
        {
          header_writer_stream_.clear();
          break;
        }
        header_writer_stream_.ignore(1, ' '); //eatup the space.
        std::getline(header_writer_stream_, line, '\n');
        if (!line.empty())
          line.resize(line.size() - 1);
        header_response_values[headerX] = line;
      }
    }
    CURL * curl_;
    curl_slist *headers_;
    std::stringstream header_writer_stream_;
    writer header_writer_;
    int response_status_code_;
    std::string response_reason_phrase_;
    std::map<std::string, std::string> header_response_values;

  };
  struct cURL_GS
  {
    cURL_GS()
    {
      curl_global_init(CURL_GLOBAL_ALL);
    }
    ~cURL_GS()
    {
      curl_global_cleanup();
    }
  };
}
}
