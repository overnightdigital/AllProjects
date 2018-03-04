 // simpletest.C version 2017-04-01
 // 
 // VU Algorithmen und Datenstrukturen - SS 2017 Universitaet Wien
 // Container - Projekt
 // https://cewebs.cs.univie.ac.at/ADS/ss17/
 //
 // Simples Testprogramm zur Ueberpruefung der Container-Funktionalitaet
 //
 // BEACHTEN:
 // (1) Der Elementdatentyp muss mit Compileroption -DETYPE=<typ> festgelegt werden,
 //     also zb -DETYPE=std::string. <typ> muss zum Testen Ein- und -Ausgabe über Streams 
 //     mittels >> bzw << unterstützen.
 // (2) Der zweite Templateparameter kann mit Compileroption -DSIZE=<n> festgelegt
 //     werden, also zb -DSIZE=13, andernfalls wird der Defaultwert verwendet.
 // (3) Das Testprogramm testet nicht alle Container-Methoden/Funktionen, es kann aber 
 //     entsprechend erweitert werden.
 // 
 // g++ -Wall -Wextra -O3 --std=c++11 --pedantic-errors -DETYPE=std::string simpletest.C
 // g++ -Wall -Wextra -O3 --std=c++11 --pedantic-errors -DETYPE=std::string -DSIZE=13 simpletest.C

 #include <iostream>
 #include <sstream>
 #include <fstream>
 #include <string>
 #include <vector>
 #include <iomanip>
 #include <iterator>
 #include <stdexcept>
 #include <set>
 #include <algorithm>
 #include <random>
 #include "ADS_set.h"
 
 #ifndef ETYPE
 #error ETYPE not defined - compile with option -DETYPE=type
 #endif

 class Person;
 using Key = ETYPE;
 using reference_set = std::set<Key>; 

 #ifdef SIZE
   using ads_set = ADS_set<Key,SIZE>;
 #else
   using ads_set = ADS_set<Key>;
 #endif

 enum class Code {quit = 0, new_set, delete_set, insert, erase, find, count, size, empty, dump, trace, finsert, ferase, rinsert, rerase, help, clear, iterator, list, iinsert, fiinsert, riinsert};  

 struct Command {
   Code code;
   std::string name, info;
   bool need_set, key_args;
 };
 std::vector<Command> commands = {
   {Code::new_set, "new", "create new set", false, false},
   {Code::dump, "dump", "call dump()", true, false},
   {Code::delete_set, "delete", "delete set", true, false},
   {Code::iinsert, "iinsert <keys>", "insert <keys>, call insert(InputIt, InputIt)", true, true},
   {Code::insert, "insert <keys>", "insert <keys>, call insert(const key_type&)", true, true},
   {Code::iterator, "iterator", "iterator/find test", true, false},
   {Code::erase, "erase [<keys>]", "erase all keys (no output) or <keys>", true, true},
   {Code::find, "find [<keys>]", "call find() for all keys (no output) or <keys>", true, true},
   {Code::count, "count [<keys>]", "call count() for all keys (no output) or <keys>", true, true},
   {Code::clear, "clear", "call clear()", true, false},
   {Code::size, "size", "call size()", true, false},
   {Code::empty, "empty", "call empty()", true, false},
   {Code::trace, "trace", "toggle tracing on/off", false, false},
   {Code::fiinsert, "fiinsert <filename>", "insert values from file <filename>, call insert(InputIt, InputIt)", true, false},
   {Code::finsert, "finsert <filename>", "insert values from file <filename>, call insert(const key_type&)", true, false},
   {Code::ferase, "ferase <filename>", "erase values from file <filename>", true, false},
   {Code::riinsert, "riinsert [<n> [<seed>]]", "insert <n> random values, call insert(InputIt, InputIt), optionally reset generator to <seed>", true, false},
   {Code::rinsert, "rinsert [<n> [<seed>]]", "insert <n> random values, call insert(const key_type&), optionally reset generator to <seed>", true, false},
   {Code::rerase, "rerase [<n> [<seed>]]", "erase <n> random values, optionally reset generator to <seed>", true, false},
   {Code::quit, "quit (or EOF)", "quit program (deletes set)", false, false},
   {Code::list, "list", "list all intended elements (sorted)", true, false},
   {Code::help, "help", "list of commands", false, false},
   {Code::help, "?", "like 'help'", false, false}
 };
 
 class Person {
   std::string vn;
   std::string nn;
 public:
   Person() = default;
   Person(const std::string& vn, const std::string& nn) : vn(vn), nn(nn) { }
   friend struct std::hash<Person>;
   friend struct std::equal_to<Person>;
   friend struct std::less<Person>;
   friend std::ostream& operator<<(std::ostream& o, const Person& p) { return o << '[' << p.nn << ", " << p.vn << ']'; }
   friend std::istream& operator>>(std::istream& i, Person& p) { return i >> p.vn >> p.nn; }
 };
 
 namespace std {
   template <> struct hash<Person> {
     size_t operator()(const Person& p) const {
       return std::hash<std::string>{}(p.vn) ^ std::hash<std::string>{}(p.nn) << 1;
     }
   };
   template <> struct equal_to<Person> {
     bool operator()(const Person& lhs, const Person& rhs) const {
       return lhs.vn == rhs.vn && lhs.nn == rhs.nn; 
     }
   };
   template <> struct less<Person> {
     bool operator()(const Person& lhs, const Person& rhs) const { 
       return lhs.nn < rhs.nn || (lhs.nn == rhs.nn && lhs.vn < rhs.vn); 
     }
   };
 }

 struct Rand {
   std::default_random_engine re;
   std::uniform_int_distribution<unsigned long> dist;
   void seed(unsigned s) { re.seed(s); }
   template <typename T> T next() { return static_cast<T>(dist(re)); }
 };
 template <> std::string Rand::next() {
   std::string rc;
   for (auto i = dist(re); i; i /= 26) rc += 'a' + i%26;
   return rc;
 }
 template <> Person Rand::next() {
   return Person(next<std::string>(), next<std::string>());
 }


 template <typename C1, typename It1, typename C2, typename It2>
 bool it_equal(const C1& c1, const It1& it1, const C2& c2, const It2& it2) {
   return (it1 == c1.end() && it2 == c2.end()) || (it1 != c1.end() && it2 != c2.end() && std::equal_to<Key>{}(*it1, *it2));
 }
 template <typename C, typename It>
 std::string it2str(const C& c, const It& it) {
   if (it == c.end()) return std::string{"end()"};
   std::stringstream buf;
   buf << '&' << *it;
   return buf.str(); 
 } 

 void test_insert(const Key& k, ads_set* c, reference_set* r, bool verbose = false) {
   auto r_rc = r->insert(k);
   auto c_rc = c->insert(k);
   if (!it_equal(*r, r_rc.first, *c, c_rc.first) || r_rc.second != c_rc.second) 
     std::cout << "\n ERROR for " << k << ", returns {" << it2str(*c, c_rc.first) << ", " << c_rc.second
       << "}, should be {" << it2str(*r, r_rc.first) << ", " << r_rc.second << "}\n";
   else if (verbose)
     std::cout << " {" << it2str(*r, r_rc.first) << ", " << c_rc.second << "}";
 }
 void test_erase(const Key& k, ads_set* c, reference_set* r = nullptr, bool verbose = false) {
   size_t r_rc {r ? r->erase(k) : 1}; 
   size_t c_rc {c->erase(k)};
   if (r_rc != c_rc) 
     std::cout << "\n ERROR for " << k << ", returns " << c_rc << ", should be " << r_rc << '\n';
   else if (verbose) 
     std::cout << ' ' << c_rc;
 }
 void test_count(const Key& k, ads_set* c, reference_set* r = nullptr, bool verbose = false) {
   size_t r_rc {r ? r->count(k) : 1}; 
   size_t c_rc {c->count(k)};
   if (r_rc != c_rc) 
     std::cout << "\n ERROR for " << k << ", returns " << c_rc << ", should be " << r_rc << '\n';
   else if (verbose)
     std::cout << ' ' << c_rc;
 }
 void test_find(const Key& k, ads_set* c, reference_set* r, bool verbose = false) {
   auto c_rc = c->find(k);
   auto r_rc = r->find(k);
   if (!it_equal(*c, c_rc, *r, r_rc))
     std::cout << "\n ERROR for " << k << ", returns " << it2str(*c, c_rc) << ", should be " << it2str(*r, r_rc) << '\n';
   else if (verbose)
     std::cout << ' ' << it2str(*c, c_rc);
 }
 
 int main() {
   Rand random;
   ads_set* c = nullptr;
   reference_set* r = nullptr;
   bool do_trace = false;
   std::cout.setf(std::ios_base::boolalpha);
 
   std::string line;
   while (std::cout << "\nsimpletest> ", std::getline(std::cin, line)) {
     std::istringstream line_stream(line);
     std::string cmd;
     line_stream >> cmd;

     if (!cmd.size()) continue;
     auto cmd_it = find_if(commands.begin(), commands.end(), [&cmd](const Command& c) {return !c.name.find(cmd);});
     if (cmd_it == commands.end()) {
       std::cout << cmd << "? ERROR - try 'help'";
       continue;
     }
     if (cmd_it->code == Code::quit) break;
     if (cmd_it->need_set && !c) {
       std::cout << "ERROR - no container (use 'new')";
       continue;
     }
     
     try {
       std::vector<Key> key_v;
       if (cmd_it->key_args) {
         for (Key key; line_stream >> key; ) key_v.push_back(key);
       }
       switch (cmd_it->code) {
         case Code::new_set:
           if (c) {
             std::cout << "ERROR - container exists, 'delete' it first";
             continue;
           }
           c = new ads_set;
           r = new reference_set;
           break;
         case Code::help: {
           std::cout << '\n' << std::right << std::setfill('.');
           size_t maxw {std::accumulate(commands.begin(), commands.end(), size_t{0}, [](const size_t& m, const Command& c) { return std::max(m,c.name.size()); })};
           for (const auto& x: commands)
             std::cout << x.name << ' ' << std::setw(maxw-x.name.size()+3) << ' ' << x.info << '\n';
           std::cout << std::setfill(' ') << "\narguments surrounded by [] are optional\n";
           break;
         }
         case Code::trace:
           std::cout << "trace " << ((do_trace = !do_trace) ? "on" : "off");
           break;
         case Code::delete_set:
           delete c, c = nullptr;
           delete r, r = nullptr;
           break;
         case Code::iinsert:
           r->insert(key_v.begin(), key_v.end()); 
           c->insert(key_v.begin(), key_v.end());
           break;
         case Code::insert:
           for (const Key& k: key_v) test_insert(k, c, r, true);
           break;
         case Code::erase:
           if (key_v.size()) for (const Key& k: key_v) test_erase(k, c, r, true);
           else {
             for (const Key& k: *r) test_erase(k, c);
             r->clear();
           }
           break;
         case Code::count:
           if (key_v.size()) for (const Key& k: key_v) test_count(k, c, r, true);
           else for (const auto& k: *r) test_count(k, c);
           break;
         case Code::find:
           if (key_v.size()) for (const Key& k: key_v) test_find(k, c, r, true);
           else for (const auto& k: *r) test_find(k, c, r);
           break;
         case Code::size:
           std::cout << " " << c->size();
           if (c->size() != r->size()) std::cout << " ERROR, should be " << r->size();
           break;
         case Code::empty:
           std::cout << " " << c->empty();
           if (c->empty() != r->empty()) std::cout << " ERROR, should be " << r->empty();
           break;
         case Code::dump:
           c->dump(std::cout);
           break;
         case Code::clear:
           r->clear();
           c->clear();
           break;
         case Code::fiinsert: {
           std::string filename;
           line_stream >> filename;
           std::ifstream is(filename);
           r->insert(std::istream_iterator<Key>{is}, std::istream_iterator<Key>{});
           is.clear(); is.seekg(0);
           c->insert(std::istream_iterator<Key>{is}, std::istream_iterator<Key>{});
           break;
         }
         case Code::finsert: {
           std::string filename;
           line_stream >> filename;
           std::ifstream is(filename);
           for (Key k; is >> k; ) test_insert(k, c, r);
           break;
         }
         case Code::ferase: {
           std::string filename;
           line_stream >> filename;
           std::ifstream is(filename);
           for (Key k; is >> k; ) test_erase(k, c, r);
           break;
         }
         case Code::riinsert: {
           unsigned seed, count{1};
           line_stream >> count;
           if (line_stream >> seed) random.seed(seed);
           std::vector<Key> v;
           while (count-- > 0) v.push_back(random.next<Key>());
           r->insert(v.begin(), v.end());
           c->insert(v.begin(), v.end());
           break;
         }
         case Code::rinsert: {
           unsigned seed, count{1};
           line_stream >> count;
           if (line_stream >> seed) random.seed(seed);
           while (count-- > 0) test_insert(random.next<Key>(), c, r);
           break;
         }
         case Code::rerase: {
           unsigned seed, count{1};
           line_stream >> count;
           if (line_stream >> seed) random.seed(seed);
           while (count-- > 0) test_erase(random.next<Key>(), c, r);
           break;
         }
         case Code::iterator: {
           std::vector<Key> v {c->begin(), c->end()};
           if (v.size() != r->size()) {
             std::cout << " ERROR vector fill failed (wrong size)";
             break;
           }
           for (auto v_it = v.begin(); v_it != v.end(); ++v_it) {
             auto c_it = c->find(*v_it);
             if (!std::equal_to<Key>{}(*c_it, *v_it) || !std::equal(c->begin(), c_it, v.begin(), std::equal_to<Key>{}) || !std::equal(c_it, c->end(), v_it, std::equal_to<Key>{})) {
               std::cout << " ERROR";
               break;
             }
           }
           break;
         }
         case Code::list:
           for (const Key& k: *r) std::cout << ' ' << k;
           break;
         default:
           throw std::runtime_error("ERROR - unknown command code");
       }
       if (do_trace && c) {
         std::cout << "\n";
         c->dump(std::cout);
       }
     } catch (std::exception& e) {
       std::cout << "ERROR - caught std::exception \"" << e.what() << "\"";
     } catch (...) {
       std::cout << "ERROR - caught something else";
     }
   }
   delete c;
   delete r;
   return 0;
 }

