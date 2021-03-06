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

#ifndef DB_BASE_H_
#define DB_BASE_H_

#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/any.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/shared_ptr.hpp>

#include "object_recognition/db/utils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace object_recognition
{
  namespace db_future
  {

    /** The main class that interact with the db
     * A collection is similar to the term used in CouchDB. It could be a schema/table in SQL
     */
    class ObjectDbBase
    {
    public:
      virtual
      ~ObjectDbBase()
      {
      }
      virtual void
      insert_object(const CollectionName &collection, const boost::property_tree::ptree &fields,
                    DocumentId & document_id, RevisionId & revision_id) = 0;

      virtual void
      persist_fields(const DocumentId & document_id, const CollectionName &collection,
                     const boost::property_tree::ptree &fields, RevisionId & revision_id) = 0;

      virtual void
      load_fields(const DocumentId & document_id, const CollectionName &collection,
                  boost::property_tree::ptree &fields) = 0;

      virtual void
      Query(const std::vector<std::string> & queries, const CollectionName & collection_name, int limit_rows,
            int start_offset, int& total_rows, int& offset, std::vector<DocumentId> & document_ids) = 0;

      virtual void
      set_attachment_stream(const DocumentId & document_id, const CollectionName &collection,
                            const AttachmentName& attachment_name, const MimeType& mime_type,
                            const std::istream& stream, RevisionId & revision_id)=0;

      virtual void
      get_attachment_stream(const DocumentId & document_id, const CollectionName &collection,
                            const AttachmentName& attachment_name, const MimeType& mime_type, std::ostream& stream,
                            RevisionId & revision_id)=0;

      virtual DbType
      type() = 0;
    };
  }
}

#endif // DB_BASE_H_
