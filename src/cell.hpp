#ifndef CELL_H
#define CELL_H

#include <type_traits>
#include <utility>
#include <string>
#include <chrono>
#include <format>

#include "date.h"

template<typename T, typename... Ts>
struct contains_type : std::false_type {};

template<typename T1, typename T2, typename... Ts>
struct contains_type<T1, T2, Ts...> : std::bool_constant<
  std::is_same<T1, T2>::value || contains_type<T1, Ts...>::value
> {};

template<typename T, typename... Ts>
constexpr bool contains_type_v = contains_type<T, Ts...>::value;

/*
 * Alternative implementation using fold expressions:
 * template<typename T, typename... Ts>
 * constexpr bool contains_type_v = (std::is_same<T, Ts>::value || ...);
*/

// Recursive templated function to call the appropriate destructor of the object
// stored in the container given its type
template<typename T, typename... Ts>
struct destructor {
  static inline void destruct(std::size_t typeID, void* object) {
    if (typeid(T).hash_code() == typeID) {
      reinterpret_cast<T*>(object)->~T();
    } else {
      destructor<Ts...>::destruct(typeID, object);
    }
  }
};

// Template specialization of above destructor declaration, necessary in order
// to terminate the recursive call chain. Recall that template parameter packs
// can accept zero template arguments
template<typename T>
struct destructor<T> {
  static inline void destruct(std::size_t typeID, void* object) {
    if (typeid(T).hash_code() == typeID) {
      reinterpret_cast<T*>(object)->~T();
    }
  }
};

template<typename T, typename... Ts>
struct copy_constructor {
  static inline void construct(void* buffer, std::size_t otherID, void const*
      otherBuffer) {
    if (typeid(T).hash_code() == otherID) {
      // For some reason, if the expression provided to reinterpret_cast is a
      // pointer, then the converted-to type must also be a pointer
      new(buffer) T(*reinterpret_cast<T const*>(otherBuffer));
    } else {
      copy_constructor<Ts...>::construct(buffer, otherID, otherBuffer);
    }
  }
};

template<typename T>
struct copy_constructor<T> {
  static inline void construct(void* buffer, std::size_t otherID, void const*
      otherBuffer) {
    if (typeid(T).hash_code() == otherID) {
      new(buffer) T(*reinterpret_cast<T const*>(otherBuffer));
    }
  }
};

template<typename... Ts>
class GenericCell {
public:
  GenericCell() : typeID{emptyType()} {}

  template<typename T, typename = std::enable_if_t<contains_type_v<T, Ts...>>>
  GenericCell(T const& value) {
    // Copy-construct the "T" object, but do not automatically allocate memory
    // for it. Instead, place it in the pre-allocated storage at memory address
    // "buffer" (placement new). This allows us to reference the object (in our
    // written code) outside of the scope of this function, where we won't know
    // the definite type of the object until it's determined by the compiler
    new (buffer) T(value);
    typeID = typeid(T).hash_code();
  }

  // The byte array buffer will automatically be de-allocated, but we must still
  // call the stored object's destructor
  ~GenericCell() { destructor<Ts...>::destruct(typeID, buffer); } // Array
								  // decays to
								  // pointer

  GenericCell(GenericCell<Ts...>&& other) : GenericCell() { // Delegating
							    // constructor
    swap(*this, other);
  }

  GenericCell(GenericCell<Ts...> const& other) : typeID{other.typeID} {
    copy_constructor<Ts...>::construct(buffer, other.typeID, other.buffer);
  }

  // Move/copy assignment operator - uses the copy-and-swap idiom. When an
  // rvalue reference argument is provided, the parameter object will be
  // move-constructed and then subsequently swapped in the function body.
  // Through this mechanism the operator overload functions as a move assignment
  GenericCell<Ts...>& operator=(GenericCell<Ts...> other) {
    swap(*this, other);
    return *this;
  }

  friend void swap(GenericCell<Ts...>& a, GenericCell<Ts...>& b) {
    using std::swap; // Default to using std::swap if none of this class's data
		     // members have swap overloads

    // Has the effect of copying the other GenericCell's stored object data into
    // this GenericCell's buffer. This may seem counterintuitive given that this
    // a move constructor; however, we still avoid
    // re-constructing/re-initializing the other GenericCell's stored object,
    // and any resources held by that object are effectively moved
    swap(a.buffer, b.buffer);

    // When parameter a has been default-constructed (e.g., when the enclosing
    // function is called by the move constructor), has the effect of changing
    // the typeID of the other GenericCell to the void-type hash code so that
    // the destructor of the stored object isn't called when unrolling
    // destructor::destruct, otherwise the stored object may be prematurely
    // destructed (and any resources it holds released/destructed) when the
    // moved-from GenericCell is destructed (the stored object should now only
    // ever be destructed when the moved-to GenericCell is destructed). The
    // implication of this is that void shall never be supplied as a type in the
    // GenericCell's parameter pack
    swap(a.typeID, b.typeID);
  }

  template<typename T, typename = std::enable_if_t<contains_type_v<T, Ts...>>>
  T as() const {
    if (typeid(T).hash_code() == typeID) {
      return *reinterpret_cast<T const*>(buffer);
    } else {
      throw std::bad_cast();
    }
  }

  template<typename T, typename = std::enable_if_t<contains_type_v<T, Ts...>>>
  T as() {
    if (typeid(T).hash_code() == typeID) {
      return *reinterpret_cast<T*>(buffer);
    } else {
      throw std::bad_cast();
    }
  }

  bool empty() const { return typeID == emptyType(); }
protected:
  // Any constexpr class data members must be static so that they can be
  // evaluated at compile time (non-static data members don't exist until their
  // enclosing class is instantiated, the occurance of which cannot always be
  // known at compile time)
  static constexpr std::size_t bufferSize = std::max({sizeof(Ts)...});

  // Declare a byte array to store the container's data. The alignment
  // requirement of each type in the parameter pack Ts is queried, then
  // re-specified and applied to the single array declaration. As per the
  // alignas spec, the largest alignment requirement (i.e., maximum alignement
  // across all types) will take effect.  The size of the array is given as the
  // maximum size across all types in the parameter pack expansion of Ts
  alignas(Ts...) std::byte buffer[bufferSize];
  std::size_t typeID;
private:
  std::size_t emptyType() { return typeid(void).hash_code(); }
};

using Amount = int64_t;

class Cell : GenericCell<std::string, Amount, std::chrono::year_month_day> {
public:
  using GenericCell::GenericCell;
  using GenericCell::as;

  template<typename T>
  T as(std::string specifier) { throw std::bad_cast(); }
};

/*
 * Domain specific specializations
*/

// Parsing specializations - since template arguments to a templated class
// constructor cannot be explicitly specified (attempting to do so, such as in
// the case of `Foo bar = Foo<MyType>();`, will instead pass the type
// argument(s) to the class rather than the constructor), parsing cannot be
// done in the constructor since there's no clean way to instruct the
// constructor what type the string should be parsed into
template<>
Amount Cell::as<Amount>(std::string parse) {
  if (typeID == typeid(std::string).hash_code()) {
    std::string value = *reinterpret_cast<std::string*>(buffer);
    int amountStart = parse.find('{');
    std::string prefix = parse.substr(0, amountStart);
    std::string suffix = parse.substr(parse.find('}') + 1);
    bool prefixFound = value.find(prefix) != std::string::npos;
    bool suffixFound = value.find(suffix) != std::string::npos;

    if (prefixFound && suffixFound) {
      int suffixSize = parse.size() - parse.find('}') - 1;
      int valueSuffixStart = value.size() - suffixSize;
      float amount = std::stof(value.substr(amountStart, valueSuffixStart));
      return amount * 100;
    } else {
      throw std::runtime_error("Error parsing \"" + value + "\": invalid "
			       "amount parse string \"" + parse + "\"");
    }
  } else {
    throw std::bad_cast();
  }
}

template<>
std::chrono::year_month_day Cell::as<std::chrono::year_month_day>(std::string
    parse) {
  if (typeID == typeid(std::string).hash_code()) {
    std::string value = *reinterpret_cast<std::string*>(buffer);
    std::istringstream dateStream{value};
    // FIXME: replace date library with std::chrono once Clang C++20 Calendar
    // extenstion is complete
    date::year_month_day date;
    date::from_stream(dateStream, parse.c_str(), date);
    // Convert date::year_month_day back to std::chrono::year_month_day
    return std::chrono::year_month_day{std::chrono::sys_days{date}};
  } else {
    throw std::bad_cast();
  }
}

// Formatting specialization
template<>
std::string Cell::as<std::string>(std::string format) {
  if (typeID == typeid(std::string).hash_code()) {
    return *reinterpret_cast<std::string*>(buffer);
  } else if (typeID == typeid(Amount).hash_code()) {
    // Format amount
    auto contents = *reinterpret_cast<Amount*>(buffer);
    float amountFloatingPoint = contents / 100;
    return std::vformat(format, std::make_format_args(amountFloatingPoint));
  } else if (typeID == typeid(std::chrono::year_month_day).hash_code()) {
    // Format date
    auto contents(*reinterpret_cast<std::chrono::year_month_day*>(buffer));
    return std::vformat("{:" + format + "}", std::make_format_args(contents));
  } else {
    throw std::bad_cast();
  }
}

#endif
