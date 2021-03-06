/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <boost/property_tree/json_parser.hpp>

#include "db_couch.h"

object_recognition::curl::cURL_GS curl_init_cleanup;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ObjectDbCouch::ObjectDbCouch(const std::string &url)
    :
      url_(url),
      json_writer_(json_writer_stream_),
      json_reader_(json_reader_stream_)
{
}

void
ObjectDbCouch::insert_object(const CollectionName &collection, const boost::property_tree::ptree &fields,
                             DocumentId & document_id, RevisionId & revision_id)
{
  upload_json(fields, url_id(collection, ""), "POST");
  GetObjectRevisionId(document_id, revision_id);
}

void
ObjectDbCouch::persist_fields(const DocumentId & document_id, const CollectionName &collection,
                              const boost::property_tree::ptree &fields, RevisionId & revision_id)
{
  precondition_id(document_id);
  upload_json(fields, url_id(collection, document_id), "PUT");
  //need to update the revision here.
  GetRevisionId(revision_id);
}

void
ObjectDbCouch::load_fields(const DocumentId & document_id, const CollectionName &collection,
                           boost::property_tree::ptree &fields)
{
  precondition_id(document_id);
  curl_.reset();
  json_writer_stream_.str("");
  curl_.setWriter(&json_writer_);

  curl_.setURL(url_id(collection, document_id));
  curl_.GET();

  curl_.perform();

  if (curl_.get_response_code() == object_recognition::curl::cURL::OK)
  {
    //update the object from the result.
    json_writer_stream_.seekg(0);
    boost::property_tree::read_json(json_writer_stream_, fields);
  }
}

void
ObjectDbCouch::set_attachment_stream(const DocumentId & document_id, const CollectionName &collection,
                                     const AttachmentName& attachment_name, const MimeType& mime_type,
                                     const std::istream& stream, RevisionId & revision_id)
{
  precondition_id(document_id);
  precondition_rev(revision_id);

  object_recognition::curl::reader binary_reader(stream);
  curl_.reset();
  curl_.setReader(&binary_reader);
  json_writer_stream_.str("");
  curl_.setWriter(&json_writer_);
  curl_.setHeader("Content-Type: " + mime_type);
  curl_.setURL(url_id(collection, document_id) + "/" + attachment_name + "?rev=" + revision_id);
  curl_.PUT();
  curl_.perform();
  GetRevisionId(revision_id);
}

void
ObjectDbCouch::get_attachment_stream(const DocumentId & document_id, const CollectionName &collection,
                                     const std::string& attachment_name, const std::string& content_type,
                                     std::ostream& stream, RevisionId & revision_id)
{
  object_recognition::curl::writer binary_writer(stream);
  curl_.reset();
  json_writer_stream_.str("");
  curl_.setWriter(&binary_writer);
  curl_.setURL(url_id(collection, document_id) + "/" + attachment_name);
  curl_.GET();
  curl_.perform();
}

void
ObjectDbCouch::GetObjectRevisionId(DocumentId& document_id, RevisionId & revision_id)
{
  boost::property_tree::ptree params;
  boost::property_tree::read_json(json_writer_stream_, params);
  document_id = params.get<std::string>("id", "");
  revision_id = params.get<std::string>("rev", "");
  if (document_id.empty())
    throw std::runtime_error("Could not find the object id");
  if (revision_id.empty())
    throw std::runtime_error("Could not find the revision number");
}

void
ObjectDbCouch::GetRevisionId(RevisionId & revision_id)
{
  boost::property_tree::ptree params;
  boost::property_tree::read_json(json_writer_stream_, params);
  revision_id = params.get<std::string>("rev", "");
  if (revision_id.empty())
    throw std::runtime_error("Could not find the revision number, from GetRevisionId");
}

void
ObjectDbCouch::Query(const std::vector<std::string> & queries, const CollectionName & collection_name, int limit_rows,
                     int start_offset, int& total_rows, int& offset, std::vector<DocumentId> & document_ids)
{
  if (limit_rows <= 0)
    limit_rows = std::numeric_limits<int>::max();
  {
    boost::property_tree::ptree fields;
    BOOST_FOREACH(const std::string& query, queries)
        {
          fields.add<std::string>("map", query);
        }
    json_reader_stream_.str("");
    boost::property_tree::write_json(json_reader_stream_, fields);
  }
  json_writer_stream_.str("");
  curl_.reset();
  curl_.setReader(&json_reader_);
  curl_.setWriter(&json_writer_);
  curl_.setURL(
      url_ + "/" + collection_name + "/_temp_view?limit=" + boost::lexical_cast<std::string>(limit_rows) + "&skip="
      + boost::lexical_cast<std::string>(start_offset));
  curl_.setHeader("Content-Type: application/json");
  curl_.setCustomRequest("POST");
  curl_.perform();

  json_reader_stream_.seekg(0);
  json_writer_stream_.seekg(0);
  //std::cout << "view res" << json_writer_stream_.str() << std::endl;
  boost::property_tree::ptree fields;
  boost::property_tree::read_json(json_writer_stream_, fields);

  total_rows = fields.get<unsigned int>("total_rows");
  document_ids.clear();
  BOOST_FOREACH(const boost::property_tree::ptree::value_type & v, fields.get_child("rows"))
      {
        // values are: id, key, value
        document_ids.push_back(v.second.get<std::string>("id"));
      }
  offset = fields.get<unsigned int>("offset") + document_ids.size();
}

void
ObjectDbCouch::upload_json(const boost::property_tree::ptree &ptree, const std::string& url, const std::string& request)
{
  curl_.reset();
  json_writer_stream_.str("");
  json_reader_stream_.str("");
  boost::property_tree::write_json(json_reader_stream_, ptree);
  curl_.setWriter(&json_writer_);
  curl_.setReader(&json_reader_);
  //couch db post to the db
  curl_.setURL(url);
  curl_.setHeader("Content-Type: application/json");
  if (request == "PUT")
  {
    curl_.PUT();
  }
  else
  {
    curl_.setCustomRequest(request.c_str());
  }
  curl_.perform();
}
