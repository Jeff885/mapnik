/*****************************************************************************
 * 
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#ifndef MAPNIK_FEATURE_HPP
#define MAPNIK_FEATURE_HPP

// mapnik
#include <mapnik/value.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/raster.hpp>
#include <mapnik/feature_kv_iterator.hpp>
// boost
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
#include <boost/property_map/property_map.hpp>
#else
#include <boost/property_map.hpp>
#endif
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

// stl
#include <vector>
#include <map>
#include <stdexcept>

namespace mapnik {

typedef boost::shared_ptr<raster> raster_ptr; 
typedef std::string key_type;   
typedef std::map<key_type,std::size_t> map_type;
typedef boost::associative_property_map<map_type> base_type;

class feature_impl;

class context : private boost::noncopyable, 
                public base_type

{
    friend class feature_impl; 
public:
    typedef map_type::value_type value_type;
    typedef map_type::key_type key_type;
    typedef map_type::size_type size_type;
    typedef map_type::difference_type difference_type;
    typedef map_type::iterator iterator;
    typedef map_type::const_iterator const_iterator;
    
    context()
        : base_type(mapping_) {}
    
    void push(key_type const& name)
    {
        mapping_.insert(std::make_pair(name,mapping_.size()));
    }
    std::size_t size() const { return mapping_.size(); }
    const_iterator begin() const { return mapping_.begin();}
    const_iterator end() const { return mapping_.end();}
    
private:
    map_type mapping_;
};

typedef boost::shared_ptr<context> context_ptr;

class feature_impl : private boost::noncopyable
{
    friend class feature_kv_iterator;
public:

    typedef mapnik::value value_type;    
    typedef std::vector<value_type> cont_type;
    typedef feature_kv_iterator iterator;
    
    feature_impl(context_ptr const& ctx, int id) 
        : id_(id),
          ctx_(ctx),
          data_(ctx_->mapping_.size())
    {}
    
    inline int id() const { return id_;}

    inline void set_id(int id) { id_ = id;}
    
    template <typename T>
    void put(key_type const& key, T const& val)
    {
        map_type::const_iterator itr = ctx_->mapping_.find(key);
        if (itr != ctx_->mapping_.end() 
            && itr->second < data_.size())
        {
            data_[itr->second] = value(val);
        }
        else 
            throw std::out_of_range("Key doesn't exist");
    } 
    
    void put(key_type const& key, value const& val)
    {
        map_type::const_iterator itr = ctx_->mapping_.find(key);
        if (itr != ctx_->mapping_.end() 
            && itr->second < data_.size())
        {
            data_[itr->second] = val;
        }
        else
            throw std::out_of_range("Key doesn't exist");
    }
    
    bool has_key(key_type const& key) const
    {
        return (ctx_->mapping_.find(key) != ctx_->mapping_.end());
    }
    
    value_type const& get(key_type const& key) const
    {
        map_type::const_iterator itr = ctx_->mapping_.find(key);
        if (itr != ctx_->mapping_.end() 
            && itr->second < data_.size())
        {
            return data_[itr->second];
        }
        throw std::out_of_range("Key doesn't exist");
    }
    
    std::size_t size() const
    {
        return data_.size();
    }
    
    context_ptr context()
    {
        return ctx_;
    }

    boost::ptr_vector<geometry_type> & paths() 
    {
        return geom_cont_;
    }
        
    void add_geometry(geometry_type * geom)
    {
        geom_cont_.push_back(geom);
    }
       
    unsigned num_geometries() const
    {
        return geom_cont_.size();
    }
    
    geometry_type const& get_geometry(unsigned index) const
    {
        return geom_cont_[index];
    }
       
    geometry_type& get_geometry(unsigned index)
    {
        return geom_cont_[index];
    }
    
    box2d<double> envelope() const
    {
        box2d<double> result;
        for (unsigned i=0;i<num_geometries();++i)
        {
            geometry_type const& geom = get_geometry(i);
            if (i==0)
            {
                box2d<double> box = geom.envelope();
                result.init(box.minx(),box.miny(),box.maxx(),box.maxy());
            }
            else
            {
                result.expand_to_include(geom.envelope());
            }
        }
        return result;
    }       
    
    const raster_ptr& get_raster() const
    {
        return raster_;
    }
    
    void set_raster(raster_ptr const& raster)
    {
        raster_ = raster;
    }

    feature_kv_iterator begin() const
    {
        return feature_kv_iterator(*this,true);
    }

    feature_kv_iterator end() const
    {
        return feature_kv_iterator(*this);
    }

    std::string to_string() const
    {        
        std::stringstream ss;
        ss << "Feature (" << std::endl;
        map_type::const_iterator itr = ctx_->mapping_.begin();
        map_type::const_iterator end = ctx_->mapping_.end();
        for ( ;itr!=end; ++itr)
        {
            ss << "  " << itr->first  << ":" <<  data_[itr->second] << std::endl;
        }
        ss << ")" << std::endl;
        return ss.str();
    }

private:
    int id_;
    context_ptr ctx_;
    boost::ptr_vector<geometry_type> geom_cont_;
    raster_ptr raster_;
    cont_type data_;
};

   
inline std::ostream& operator<< (std::ostream & out,feature_impl const& f)
{
    out << f.to_string();
    return out;
}

typedef feature_impl Feature;

}

#endif // MAPNIK_FEATURE_HPP
