/* A small standalone program to test whether the permuter works
*/

#include "outcome/outcome.hpp"

#include "quickcpplib/bitfield.hpp"
#include <filesystem>

namespace minimal_afio
{
  using namespace OUTCOME_V2_NAMESPACE;
  struct native_handle_type
  {
    HANDLE h;
    native_handle_type()
        : h(nullptr)
    {
    }
  };
  struct file_handle
  {
    using path_type = std::experimental::filesystem::path;
    enum class mode : unsigned char
    {
      unchanged = 0,
      none = 2,        //!< No ability to read or write anything, but can synchronise (SYNCHRONIZE or 0)
      attr_read = 4,   //!< Ability to read attributes (FILE_READ_ATTRIBUTES|SYNCHRONIZE or O_RDONLY)
      attr_write = 5,  //!< Ability to read and write attributes (FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE or O_RDONLY)
      read = 6,        //!< Ability to read (READ_CONTROL|FILE_READ_DATA|FILE_READ_ATTRIBUTES|FILE_READ_EA|SYNCHRONISE or O_RDONLY)
      write = 7,       //!< Ability to read and write (READ_CONTROL|FILE_READ_DATA|FILE_READ_ATTRIBUTES|FILE_READ_EA|FILE_WRITE_DATA|FILE_WRITE_ATTRIBUTES|FILE_WRITE_EA|FILE_APPEND_DATA|SYNCHRONISE or O_RDWR)
      append = 9       //!< All mainstream OSs and CIFS guarantee this is atomic with respect to all other appenders (FILE_APPEND_DATA|SYNCHRONISE or O_APPEND)
    };
    enum class creation : unsigned char
    {
      open_existing = 0,
      only_if_not_exist,
      if_needed,
      truncate  //!< Atomically truncate on open, leaving creation date unmodified.
    };
    enum class caching : unsigned char  // bit 0 set means safety fsyncs enabled
    {
      unchanged = 0,
      none = 1,                //!< No caching whatsoever, all reads and writes come from storage (i.e. <tt>O_DIRECT|O_SYNC</tt>). Align all i/o to 4Kb boundaries for this to work. <tt>flag_disable_safety_fsyncs</tt> can be used here.
      only_metadata = 2,       //!< Cache reads and writes of metadata but avoid caching data (<tt>O_DIRECT</tt>), thus i/o here does not affect other cached data for other handles. Align all i/o to 4Kb boundaries for this to work.
      reads = 3,               //!< Cache reads only. Writes of data and metadata do not complete until reaching storage (<tt>O_SYNC</tt>). <tt>flag_disable_safety_fsyncs</tt> can be used here.
      reads_and_metadata = 5,  //!< Cache reads and writes of metadata, but writes of data do not complete until reaching storage (<tt>O_DSYNC</tt>). <tt>flag_disable_safety_fsyncs</tt> can be used here.
      all = 4,                 //!< Cache reads and writes of data and metadata so they complete immediately, sending writes to storage at some point when the kernel decides (this is the default file system caching on a system).
      safety_fsyncs = 7,       //!< Cache reads and writes of data and metadata so they complete immediately, but issue safety fsyncs at certain points. See documentation for <tt>flag_disable_safety_fsyncs</tt>.
      temporary = 6            //!< Cache reads and writes of data and metadata so they complete immediately, only sending any updates to storage on last handle close in the system or if memory becomes tight as this file is expected to be temporary (Windows only).
                               // NOTE: IF UPDATING THIS UPDATE THE std::ostream PRINTER BELOW!!!
    };
    QUICKCPPLIB_BITFIELD_BEGIN(flag)
    {
      none = 0,  //!< No flags
      unlink_on_close = 1 << 0, disable_safety_fsyncs = 1 << 2, disable_safety_unlinks = 1 << 3,

      win_disable_unlink_emulation = 1 << 24,  //!< See the documentation for `unlink_on_close`

      overlapped = 1 << 28,         //!< On Windows, create any new handles with OVERLAPPED semantics
      byte_lock_insanity = 1 << 29  //!< Using insane POSIX byte range locks
    }
    QUICKCPPLIB_BITFIELD_END(flag)
    file_handle(path_type path, caching, flag)
        : _path(std::move(path))

    {
    }
    native_handle_type _v;
    path_type _path;
  };
  using handle = file_handle;
  static inline result<ACCESS_MASK> access_mask_from_handle_mode(handle::mode _mode)
  {
    ACCESS_MASK access = SYNCHRONIZE;
    switch(_mode)
    {
    case handle::mode::unchanged:
      return std::errc::invalid_argument;
    case handle::mode::none:
      break;
    case handle::mode::attr_read:
      access |= FILE_READ_ATTRIBUTES;
      break;
    case handle::mode::attr_write:
      access |= FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
      break;
    case handle::mode::read:
      access |= GENERIC_READ;
      break;
    case handle::mode::write:
      access |= GENERIC_WRITE | GENERIC_READ;
      break;
    case handle::mode::append:
      access |= FILE_APPEND_DATA;
      break;
    }
    return access;
  }
  static inline result<DWORD> attributes_from_handle_caching_and_flags(handle::caching _caching, handle::flag flags)
  {
    DWORD attribs = 0;
    if(flags & handle::flag::overlapped)
    {
      attribs |= FILE_FLAG_OVERLAPPED;
    }
    switch(_caching)
    {
    case handle::caching::unchanged:
      return std::errc::invalid_argument;
    case handle::caching::none:
      attribs |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
      break;
    case handle::caching::only_metadata:
      attribs |= FILE_FLAG_NO_BUFFERING;
      break;
    case handle::caching::reads:
    case handle::caching::reads_and_metadata:
      attribs |= FILE_FLAG_WRITE_THROUGH;
      break;
    case handle::caching::all:
    case handle::caching::safety_fsyncs:
      break;
    case handle::caching::temporary:
      attribs |= FILE_ATTRIBUTE_TEMPORARY;
      break;
    }
    if(flags & handle::flag::unlink_on_close)
      attribs |= FILE_FLAG_DELETE_ON_CLOSE;
    return attribs;
  }
}

namespace file_create
{
  using namespace minimal_afio;
  using QUICKCPPLIB_NAMESPACE::ringbuffer_log::last190;
  extern BOOST_SYMBOL_EXPORT result<file_handle> test_file_create(file_handle::path_type _path, file_handle::mode _mode = file_handle::mode::read, file_handle::creation _creation = file_handle::creation::open_existing, file_handle::caching _caching = file_handle::caching::all,
                                                                  file_handle::flag flags = file_handle::flag::none) noexcept
  {
    result<file_handle> ret(file_handle(std::move(_path), _caching, flags));
    native_handle_type &nativeh = ret.get()._v;
    BOOST_OUTCOME_FILTER_ERROR(access, access_mask_from_handle_mode(_mode));
    DWORD creation = OPEN_EXISTING;
    switch(_creation)
    {
    case file_handle::creation::open_existing:
      break;
    case file_handle::creation::only_if_not_exist:
      creation = CREATE_NEW;
      break;
    case file_handle::creation::if_needed:
      creation = OPEN_ALWAYS;
      break;
    case file_handle::creation::truncate:
      creation = TRUNCATE_EXISTING;
      break;
    }
    BOOST_OUTCOME_FILTER_ERROR(attribs, attributes_from_handle_caching_and_flags(_caching, flags));
    if(INVALID_HANDLE_VALUE == (nativeh.h = CreateFile(ret.value()._path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, creation, attribs, NULL)))
    {
      DWORD errcode = GetLastError();
      // assert(false);
      return {errcode, std::system_category()};
    }
    if(flags & file_handle::flag::unlink_on_close)
    {
      // Hide this item
      SetFileAttributes(ret.value()._path.c_str(), FILE_ATTRIBUTE_HIDDEN);
    }
    return ret;
  }
}
