/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef testgen_testgen_hh__
#define testgen_testgen_hh__

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include "Boost.hh"
#include "Exception.hh"
#include "AvroSerialize.hh"
#include "AvroParse.hh"
#include "Layout.hh"

namespace testgen {

struct Nested;

/*----------------------------------------------------------------------------------*/

struct Nested {

    Nested () :
        inval1(),
        inval2(),
        inval3()
    { }

    double inval1;
    std::string inval2;
    int32_t inval3;
};

template <typename Serializer>
inline void serialize(Serializer &s, const Nested &val, const boost::true_type &) {
    s.writeRecord();
    serialize(s, val.inval1);
    serialize(s, val.inval2);
    serialize(s, val.inval3);
    s.writeRecordEnd();
}

template <typename Parser>
inline void parse(Parser &p, Nested &val, const boost::true_type &) {
    p.readRecord();
    parse(p, val.inval1);
    parse(p, val.inval2);
    parse(p, val.inval3);
    p.readRecordEnd();
}

class Nested_Layout : public avro::CompoundLayout {
  public:
    Nested_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(Nested, inval1)));
        add(new avro::PrimitiveLayout(offset + offsetof(Nested, inval2)));
        add(new avro::PrimitiveLayout(offset + offsetof(Nested, inval3)));
    }
}; 


/*----------------------------------------------------------------------------------*/

struct Map_of_int {
    typedef int32_t ValueType;
    typedef std::map<std::string, ValueType> MapType;
    typedef ValueType* (*GenericSetter)(Map_of_int *, const std::string &);
    
    Map_of_int() :
        value(),
        genericSetter(&Map_of_int::genericSet)
    { }

    void addValue(const std::string &key, const ValueType &val) {
        value.insert(MapType::value_type(key, val));
    }

    static ValueType *genericSet(Map_of_int *map, const std::string &key) { 
        map->value[key] = ValueType();
        return &(map->value[key]);
    }

    MapType value;
    GenericSetter genericSetter;

};

template <typename Serializer>
inline void serialize(Serializer &s, const Map_of_int &val, const boost::true_type &) {
    if(val.value.size()) {
        s.writeMapBlock(val.value.size());
        Map_of_int::MapType::const_iterator iter = val.value.begin();
        Map_of_int::MapType::const_iterator end  = val.value.end();
        while(iter!=end) {
            serialize(s, iter->first);
            serialize(s, iter->second);
            ++iter;
        }
    }
    s.writeMapEnd();
}

template <typename Parser>
inline void parse(Parser &p, Map_of_int &val, const boost::true_type &) {
    val.value.clear();
    while(1) {
        int size = p.readMapBlockSize();
        if(size > 0) {
            while (size-- > 0) { 
                std::string key;
                parse(p, key);
                Map_of_int::ValueType m;
                parse(p, m);
                val.value.insert(Map_of_int::MapType::value_type(key, m));
            }
        }
        else {
            break;
        }
    } 
}

class Map_of_int_Layout : public avro::CompoundLayout {
  public:
    Map_of_int_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(Map_of_int, genericSetter)));
        add(new avro::PrimitiveLayout);
    }
}; 


/*----------------------------------------------------------------------------------*/

struct Array_of_double {
    typedef double ValueType;
    typedef std::vector<ValueType> ArrayType;
    typedef ValueType* (*GenericSetter)(Array_of_double *);
    
    Array_of_double() :
        value(),
        genericSetter(&Array_of_double::genericSet)
    { }

    static ValueType *genericSet(Array_of_double *array) {
        array->value.push_back(ValueType());
        return &array->value.back();
    }

    void addValue(const ValueType &val) {
        value.push_back(val);
    }

    ArrayType value;
    GenericSetter genericSetter;

};

template <typename Serializer>
inline void serialize(Serializer &s, const Array_of_double &val, const boost::true_type &) {
    const size_t size = val.value.size();
    if(size) {
        s.writeArrayBlock(size);
        for(size_t i = 0; i < size; ++i) {
            serialize(s, val.value[i]);
        }
    }
    s.writeArrayEnd();
}

template <typename Parser>
inline void parse(Parser &p, Array_of_double &val, const boost::true_type &) {
    val.value.clear();
    while(1) {
        int size = p.readArrayBlockSize();
        if(size > 0) {
            val.value.reserve(val.value.size() + size);
            while (size-- > 0) { 
                val.value.push_back(Array_of_double::ValueType());
                parse(p, val.value.back());
            }
        }
        else {
            break;
        }
    } 
}

class Array_of_double_Layout : public avro::CompoundLayout {
  public:
    Array_of_double_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(Array_of_double, genericSetter)));
        add(new avro::PrimitiveLayout);
    }
}; 


/*----------------------------------------------------------------------------------*/

struct ExampleEnum {

    enum EnumSymbols {
        zero, one, two, three
    };

    ExampleEnum() : 
        value(zero) 
    { }

    EnumSymbols value;
};

template <typename Serializer>
inline void serialize(Serializer &s, const ExampleEnum &val, const boost::true_type &) {
    s.writeEnum(val.value);
}

template <typename Parser>
inline void parse(Parser &p, ExampleEnum &val, const boost::true_type &) {
    val.value = static_cast<ExampleEnum::EnumSymbols>(p.readEnum());
}

class ExampleEnum_Layout : public avro::CompoundLayout {
  public:
    ExampleEnum_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(ExampleEnum, value)));
    }
}; 


/*----------------------------------------------------------------------------------*/

struct Union_of_null_Map_of_int_float {

    typedef avro::Null T0;
    typedef Map_of_int T1;
    typedef float T2;

    typedef void* (*GenericSetter)(Union_of_null_Map_of_int_float *, int64_t);

    Union_of_null_Map_of_int_float() : 
        choice(0), 
        value(T0()),
        genericSetter(&Union_of_null_Map_of_int_float::genericSet)
    { }

    void set_null(const avro::Null &val) {
        choice = 0;
        value =  val;
    };
    void set_Map_of_int(const Map_of_int &val) {
        choice = 1;
        value =  val;
    };
    void set_float(const float &val) {
        choice = 2;
        value =  val;
    };

#ifdef AVRO_BOOST_NO_ANYREF
    template<typename T>
    const T &getValue() const {
        const T *ptr = boost::any_cast<T>(&value);
        return *ptr;
    }
#else
    template<typename T>
    const T &getValue() const {
        return boost::any_cast<const T&>(value);
    }
#endif

    static void *genericSet(Union_of_null_Map_of_int_float *u, int64_t choice) {
        boost::any *val = &(u->value);
        void *data = NULL;
        switch (choice) {
          case 0:
            *val = T0();
            data = boost::any_cast<T0>(val);
            break;
          case 1:
            *val = T1();
            data = boost::any_cast<T1>(val);
            break;
          case 2:
            *val = T2();
            data = boost::any_cast<T2>(val);
            break;
        }
        return data;
    }

    int64_t choice; 
    boost::any value;
    GenericSetter genericSetter;
};

template <typename Serializer>
inline void serialize(Serializer &s, const Union_of_null_Map_of_int_float &val, const boost::true_type &) {
    s.writeUnion(val.choice);
    switch(val.choice) {
      case 0:
        serialize(s, val.getValue< avro::Null >());
        break;
      case 1:
        serialize(s, val.getValue< Map_of_int >());
        break;
      case 2:
        serialize(s, val.getValue< float >());
        break;
      default :
        throw avro::Exception("Unrecognized union choice");
    }
}

template <typename Parser>
inline void parse(Parser &p, Union_of_null_Map_of_int_float &val, const boost::true_type &) {
    val.choice = p.readUnion();
    switch(val.choice) {
      case 0:
        { avro::Null chosenVal; parse(p, chosenVal); val.value = chosenVal; }
        break;
      case 1:
        { Map_of_int chosenVal; parse(p, chosenVal); val.value = chosenVal; }
        break;
      case 2:
        { float chosenVal; parse(p, chosenVal); val.value = chosenVal; }
        break;
      default :
        throw avro::Exception("Unrecognized union choice");
    }
}

class Union_of_null_Map_of_int_float_Layout : public avro::CompoundLayout {
  public:
    Union_of_null_Map_of_int_float_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(Union_of_null_Map_of_int_float, choice)));
        add(new avro::PrimitiveLayout(offset + offsetof(Union_of_null_Map_of_int_float, genericSetter)));
        add(new avro::PrimitiveLayout);
        add(new Map_of_int_Layout);
        add(new avro::PrimitiveLayout);
    }
}; 


/*----------------------------------------------------------------------------------*/

struct Union_of_bytes_null {

    typedef std::vector<uint8_t> T0;
    typedef avro::Null T1;

    typedef void* (*GenericSetter)(Union_of_bytes_null *, int64_t);

    Union_of_bytes_null() : 
        choice(0), 
        value(T0()),
        genericSetter(&Union_of_bytes_null::genericSet)
    { }

    void set_bytes(const std::vector<uint8_t> &val) {
        choice = 0;
        value =  val;
    };
    void set_null(const avro::Null &val) {
        choice = 1;
        value =  val;
    };

#ifdef AVRO_BOOST_NO_ANYREF
    template<typename T>
    const T &getValue() const {
        const T *ptr = boost::any_cast<T>(&value);
        return *ptr;
    }
#else
    template<typename T>
    const T &getValue() const {
        return boost::any_cast<const T&>(value);
    }
#endif

    static void *genericSet(Union_of_bytes_null *u, int64_t choice) {
        boost::any *val = &(u->value);
        void *data = NULL;
        switch (choice) {
          case 0:
            *val = T0();
            data = boost::any_cast<T0>(val);
            break;
          case 1:
            *val = T1();
            data = boost::any_cast<T1>(val);
            break;
        }
        return data;
    }

    int64_t choice; 
    boost::any value;
    GenericSetter genericSetter;
};

template <typename Serializer>
inline void serialize(Serializer &s, const Union_of_bytes_null &val, const boost::true_type &) {
    s.writeUnion(val.choice);
    switch(val.choice) {
      case 0:
        serialize(s, val.getValue< std::vector<uint8_t> >());
        break;
      case 1:
        serialize(s, val.getValue< avro::Null >());
        break;
      default :
        throw avro::Exception("Unrecognized union choice");
    }
}

template <typename Parser>
inline void parse(Parser &p, Union_of_bytes_null &val, const boost::true_type &) {
    val.choice = p.readUnion();
    switch(val.choice) {
      case 0:
        { std::vector<uint8_t> chosenVal; parse(p, chosenVal); val.value = chosenVal; }
        break;
      case 1:
        { avro::Null chosenVal; parse(p, chosenVal); val.value = chosenVal; }
        break;
      default :
        throw avro::Exception("Unrecognized union choice");
    }
}

class Union_of_bytes_null_Layout : public avro::CompoundLayout {
  public:
    Union_of_bytes_null_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(Union_of_bytes_null, choice)));
        add(new avro::PrimitiveLayout(offset + offsetof(Union_of_bytes_null, genericSetter)));
        add(new avro::PrimitiveLayout);
        add(new avro::PrimitiveLayout);
    }
}; 


/*----------------------------------------------------------------------------------*/

struct md5 {
    enum {
        fixedSize = 16
    };

    md5() {
        memset(value, 0, sizeof(value));
    }
    
    uint8_t value[fixedSize];
};

template <typename Serializer>
inline void serialize(Serializer &s, const md5 &val, const boost::true_type &) {
    s.writeFixed(val.value);
}

template <typename Parser>
inline void parse(Parser &p, md5 &val, const boost::true_type &) {
    p.readFixed(val.value);
}

class md5_Layout : public avro::CompoundLayout {
  public:
    md5_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(md5, value)));
    }
}; 


/*----------------------------------------------------------------------------------*/

struct RootRecord {

    RootRecord () :
        mylong(),
        nestedrecord(),
        mymap(),
        myarray(),
        myenum(),
        myunion(),
        anotherunion(),
        mybool(),
        anothernested(),
        myfixed(),
        anotherint(),
        bytes()
    { }

    int64_t mylong;
    Nested nestedrecord;
    Map_of_int mymap;
    Array_of_double myarray;
    ExampleEnum myenum;
    Union_of_null_Map_of_int_float myunion;
    Union_of_bytes_null anotherunion;
    bool mybool;
    Nested anothernested;
    md5 myfixed;
    int32_t anotherint;
    std::vector<uint8_t> bytes;
};

template <typename Serializer>
inline void serialize(Serializer &s, const RootRecord &val, const boost::true_type &) {
    s.writeRecord();
    serialize(s, val.mylong);
    serialize(s, val.nestedrecord);
    serialize(s, val.mymap);
    serialize(s, val.myarray);
    serialize(s, val.myenum);
    serialize(s, val.myunion);
    serialize(s, val.anotherunion);
    serialize(s, val.mybool);
    serialize(s, val.anothernested);
    serialize(s, val.myfixed);
    serialize(s, val.anotherint);
    serialize(s, val.bytes);
    s.writeRecordEnd();
}

template <typename Parser>
inline void parse(Parser &p, RootRecord &val, const boost::true_type &) {
    p.readRecord();
    parse(p, val.mylong);
    parse(p, val.nestedrecord);
    parse(p, val.mymap);
    parse(p, val.myarray);
    parse(p, val.myenum);
    parse(p, val.myunion);
    parse(p, val.anotherunion);
    parse(p, val.mybool);
    parse(p, val.anothernested);
    parse(p, val.myfixed);
    parse(p, val.anotherint);
    parse(p, val.bytes);
    p.readRecordEnd();
}

class RootRecord_Layout : public avro::CompoundLayout {
  public:
    RootRecord_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(RootRecord, mylong)));
        add(new Nested_Layout(offset + offsetof(RootRecord, nestedrecord)));
        add(new Map_of_int_Layout(offset + offsetof(RootRecord, mymap)));
        add(new Array_of_double_Layout(offset + offsetof(RootRecord, myarray)));
        add(new ExampleEnum_Layout(offset + offsetof(RootRecord, myenum)));
        add(new Union_of_null_Map_of_int_float_Layout(offset + offsetof(RootRecord, myunion)));
        add(new Union_of_bytes_null_Layout(offset + offsetof(RootRecord, anotherunion)));
        add(new avro::PrimitiveLayout(offset + offsetof(RootRecord, mybool)));
        add(new Nested_Layout(offset + offsetof(RootRecord, anothernested)));
        add(new md5_Layout(offset + offsetof(RootRecord, myfixed)));
        add(new avro::PrimitiveLayout(offset + offsetof(RootRecord, anotherint)));
        add(new avro::PrimitiveLayout(offset + offsetof(RootRecord, bytes)));
    }
}; 



} // namespace testgen

namespace avro {

template <> struct is_serializable<testgen::RootRecord> : public boost::true_type{};
template <> struct is_serializable<testgen::ExampleEnum> : public boost::true_type{};
template <> struct is_serializable<testgen::Nested> : public boost::true_type{};
template <> struct is_serializable<testgen::Map_of_int> : public boost::true_type{};
template <> struct is_serializable<testgen::Array_of_double> : public boost::true_type{};
template <> struct is_serializable<testgen::Union_of_null_Map_of_int_float> : public boost::true_type{};
template <> struct is_serializable<testgen::Union_of_bytes_null> : public boost::true_type{};
template <> struct is_serializable<testgen::md5> : public boost::true_type{};

} // namespace avro

#endif // testgen_testgen_hh__
