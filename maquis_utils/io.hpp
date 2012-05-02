#ifndef MAQUIS_IO_HPP
#define MAQUIS_IO_HPP

namespace maquis {

#ifdef AMBIENT_IO
    using ambient::cout;
    using ambient::cerr;
#else
    using std::cout;
    using std::cerr;
#endif

}

#endif
