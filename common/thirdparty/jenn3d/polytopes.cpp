/*
This file is part of Jenn.
Copyright 2001-2007 Fritz Obermeyer.

Jenn is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Jenn is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Jenn; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "polytopes.h"
#include "todd_coxeter.h"
//#include "drawing.h"
//#include "projection.h"
#include <cstring>

namespace jenn {

  namespace Polytope
  {

    Word int2word(int g)
    {
      Word result;
      while (g) {
        result.push_back((g % 10) - 1);
        g /= 10;
      }
      return result;
    }
    Word str2word(const char* g)
    {
      Word result;
      char num[2] = { 0, 0 };
      int I = (int)strlen(g);
      for (int i = 0; i < I; ++i) {
        num[0] = g[i];
        result.push_back(atoi(num));
      }
      return result;
    }
    Vect int2Vect(int w)
    {
      Vect result;
      for (int i = 0; i < 4; ++i) {
        result[3 - i] = (float)(w % 10);
        w /= 10;
      }
      return result;
    }

    ToddCoxeter::Graph* view(int c12, int c13, int c14, int c23, int c24, int c34,
      int g1, int g2, int g3,
      int edges, int faces, int weights);

    ToddCoxeter::Graph* select(int code, int edges, int faces, int weights)
    {//selects based on digit pattern CCCCCCGGG (WARNING: pack g's with zeros)
      if (not code) return NULL;

      int g3 = code % 10; code /= 10;
      int g2 = code % 10; code /= 10;
      int g1 = code % 10; code /= 10;

      int c34 = code % 10; code /= 10;
      int c24 = code % 10; code /= 10;
      int c23 = code % 10; code /= 10;
      int c14 = code % 10; code /= 10;
      int c13 = code % 10; code /= 10;
      int c12 = code % 10; code /= 10;

      return view(c12, c13, c14, c23, c24, c34,
        g1, g2, g3,
        edges, faces, weights);
    }

    ToddCoxeter::Graph* view(int c12, int c13, int c14, int c23, int c24, int c34,
      int g1, int g2, int g3,
      int edges, int faces, int weights)
    {
      /*
      logger.debug() << "viewing <"
      << c12 << ", "            //4
      << c13 << ", "            //2
      << c14 << ", "            //2
      << c23 << ", "            //3
      << c24 << ", "            //2
      << c34 << "> modulo {"    //3
      << g1 << ", "             //2
      << g2 << ", "             //3
      << g3 << "}" |0;          //4
      logger.debug() << "edges = " << edges << ", faces = " << faces |0;
      */

      int arg_coxeter[6];
      arg_coxeter[0] = c12;
      arg_coxeter[1] = c13;
      arg_coxeter[2] = c14;
      arg_coxeter[3] = c23;
      arg_coxeter[4] = c24;
      arg_coxeter[5] = c34;

      int e[5] = { 0, 0, 0, 0, 0 }; //so zero index does nothing
      Assert(0 <= g1 and g1 < 5, "generator #1 out of range");  e[g1] = true;
      Assert(0 <= g2 and g2 < 5, "generator #2 out of range");  e[g2] = true;
      Assert(0 <= g3 and g3 < 5, "generator #3 out of range");  e[g3] = true;

      //define symmetry subgroup
      std::vector<Word> gens;
      for (int i = 0; i < 4; ++i) {
        //LATER: this does nothing yet
        gens.push_back(Word(1, i));
      }

      //define vertex stabiliizer subgroup generators
      std::vector<Word> v_cogens;
      for (int i = 0; i < 4; ++i) {
        if (e[i + 1]) v_cogens.push_back(int2word(i + 1));
      }

      //define edges generators
      std::vector<Word> e_gens;
      for (int i = 0; i < 4; ++i) {
        if ((not e[i + 1]) and(edges % 10)) {
          e_gens.push_back(int2word(i + 1));
        }
        edges /= 10;
      }

      //define face generators
      std::vector<Word> f_gens;
      for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
          if (faces % 10) {
            Word word;
            word.push_back(i);
            word.push_back(j);
            f_gens.push_back(word);
          }
          faces /= 10;
        }
      }

      //define weights
      Vect weight_vect = int2Vect(weights);

      return view(arg_coxeter, gens, v_cogens, e_gens, f_gens, weight_vect);
    }

    ToddCoxeter::Graph* view(const int* coxeter,
      const WordList& gens,
      const WordList& v_cogens,
      const WordList& e_gens,
      const WordList& f_gens,
      const Vect& weights)
    {
      const Logging::fake_ostream& os = logger.debug() << "setting polytope:";
      os << "\n  gens = ";
      for (unsigned i = 0; i < gens.size(); ++i) {
        os << gens[i] << ", ";
      }
      os << "\n  v_cogens = ";
      for (unsigned i = 0; i < v_cogens.size(); ++i) {
        os << v_cogens[i] << ", ";
      }
      os << "\n  e_gens = ";
      for (unsigned i = 0; i < e_gens.size(); ++i) {
        os << e_gens[i] << ", ";
      }
      os << "\n  f_gens = ";
      for (unsigned i = 0; i < f_gens.size(); ++i) {
        os << f_gens[i] << ", ";
      }
      os | 0;

      return new ToddCoxeter::Graph(coxeter, gens, v_cogens, e_gens, f_gens, weights);
    }

    const char* phedra_message =
      "tetrahedron\ncube\noctahedron\ndodecahedron\nicosahedron";
    const int phedra_nums[5] = {
      322322234, 422322234, 322422234, 522322234, 322522234 };
    const char* pchora_message =
      "5-cell\n8-cell\n16-cell\n24-cell\n120-cell\n600-cell";
    const int pchora_nums[6] = {
      322323234, 422323234, 322324234, 322423234, 522323234, 322325234 };
    const char* dprism_message =
      "  3x3\n  4x4\n  4x5\n  4x6\n  5x5\n  4x10\n12x12\n18x18";
    const int dprism_nums[8] = {
      322223014, 222222000, 222225004, 222223000,
      522225014, 222225000, 622226000, 922229000 };
    const char* thedra_message =
      "t. tetra.\nt. cube\nt. octa.\nt. dodeca.\nt. icosa.";
    const int thedra_nums[5] = {
      322322034, 422322034, 322422034, 322522034, 522322034 };
    const char* tchora_message =
      "t. 5-cell\nt. 16-cell\nt. 8-cell\nt. 24-cell\nt. 120-cell\nt. 600-cell";
    const int tchora_nums[6] = {
      422323034, 322323034, 322324034, 322423034, 522323034, 322325034 };
    const char* bchora_message =
      "bt.. 5-cell\nbt. 8-cell\nbt. 24-cell\nbt. 120-cell";
    const int bchora_nums[4] = {
      322323014, 422323014, 322423014, 522323014 };
    const char* ehedra_message =
      "e.t. tetra.\ne.t. octa.\ne.t. cube\ne.t. dodeca.\ne.t. icosa.";
    const int ehedra_nums[5] = {
      322322024, 422322024, 322422024, 322522024, 522322024 };
    const char* echora_message =
      "e.t. 5-cell\ne.t. 8-cell\ne.t. 16-cell\ne.t. 24-cell\ne.t. 120-cell\ne.t. 600-cell";
    const int echora_nums[6] = {
      322323024, 422323024, 322324024, 322423024, 522323024, 322325024 };
    const char* cayley_message =
      "2-2-2\n2-2-3\n2-2-5\n2-2-9\n2-3-3\n2-3-4\n2-3-5\n3-3-3\n    Y\n3-3-4\n3-4-3\n3-3-5";
    const int cayley_nums[12] = {
      222222000, 322222000, 522222000, 922222000, 322322000, 422322000,
      522322000, 322323000, 333222000, 422323000, 322423000, 522323000 };

    const int fam222_nums[15] = {
      222222000, 222222001, 222222002, 222222003, 222222004,
      222222012, 222222013, 222222014, 222222023, 222222024,
      222222034, 222222123, 222222124, 222222134, 722222234 };
    const int fam227_nums[15] = {
      722222000, 722222001, 722222002, 722222003, 722222004,
      722222012, 722222013, 722222014, 722222023, 722222024,
      722222034, 722222123, 722222124, 722222134, 722222234 };
    const int fam233_nums[15] = {
      322322000, 322322001, 322322002, 322322003, 322322004,
      322322012, 322322013, 322322014, 322322023, 322322024,
      322322034, 322322123, 322322124, 322322134, 322322234 };
    const int fam234_nums[15] = {
      422322000, 422322001, 422322002, 422322003, 422322004,
      422322012, 422322013, 422322014, 422322023, 422322024,
      422322034, 422322123, 422322124, 422322134, 422322234 };
    const int fam235_nums[15] = {
      522322000, 522322001, 522322002, 522322003, 522322004,
      522322012, 522322013, 522322014, 522322023, 522322024,
      522322034, 522322123, 522322124, 522322134, 522322234 };
    const int fam333_nums[15] = {
      322323000, 322323001, 322323002, 322323003, 322323004,
      322323012, 322323013, 322323014, 322323023, 322323024,
      322323034, 322323234, 322323124, 322323134, 322323234 };
    const int fam_Y__nums[15] = {
      333222000, 333222001, 333222002, 333222003, 333222004,
      333222012, 333222013, 333222014, 333222023, 333222024,
      333222034, 333222124, 333222124, 333222134, 333222234 };
    const int fam334_nums[15] = {
      422323000, 422323001, 422323002, 422323003, 422323004,
      422323012, 422323013, 422323014, 422323023, 422323024,
      422323034, 322324234, 422323124, 422323134, 422323234 };
    const int fam343_nums[15] = {
      322423000, 322423001, 322423002, 322423003, 322423004,
      322423012, 322423013, 322423014, 322423023, 322423024,
      322423034, 322423234, 322423124, 322423134, 322423234 };
    const int fam335_nums[15] = {
      522323000, 522323001, 522323002, 522323003, 522323004,
      522323012, 522323013, 522323014, 522323023, 522323024,
      522323034, 322325234, 522323124, 522323134, 522323234 };

    const char* solids_message =
      "torus\ncubes\n8-hedra\n12-hedra\n20-hedra\n4-hedra\n6-prisms\n8-prisms";
    const int solids_nums[8] = {
      722227000, 422323023, 322324034, 522323023,
      322325034, 522323034, 322423000, 422323000 };
    //in the following, '8' is only a placeholder in 8xxxx
    const int solids_edges[8] = {
      81111, 80001, 80010, 80001, 80010, 80010, 81011, 81011 };
    const int solids_faces[8] = {
      8011110, 8000001, 8001000, 8000001, 8001000, 8001000, 8010101, 8010101 };
    const int solids_weights[8] = { 1111, 1111, 1111, 1222, 2333, 1111, 1111, 1111 };
    const char* mazes_message = "{3,5}\n{3,3,3}\n{3,3,4}\n{3,4,3}\n{3,3,5}a\n{3,3,5}b\n{3,3,5}c\n{3,3,5}d";
    const int mazes_nums[8] = {
      322522000, 322323000, 422323000, 322423000,
      522323013, 522323004, 522323003, 522323000 };
    const int mazes_edges[8] = { 1111, 1111, 1111, 1111, 1111, 1111, 1111, 1111 };
    const int mazes_faces[8] = {
      8110011, 8011110, 8011110, 8110011, 8011000, 8011110, 8011110, 8011110 };
    const int mazes_weights[8] = { 1111, 1111, 1111, 1111, 2111, 1111, 1111, 2111 };

    bool selectNextSubFamily(int& inSubtype, const char* message, int numCodes, const int* codes,
        int& outCode) {
      if(inSubtype < numCodes) {
        outCode = codes[inSubtype];
        return true;
      }
      return false;
    }

    bool selectNextSubFamily(int& inSubtype, const char* message, int numCodes,
        const int* codes, const int* edges, const int* faces, const int* weights,
        int& outCode, int& outEdges, int& outFaces, int& outWeights) {
      if(inSubtype < numCodes) {
        outCode = codes[inSubtype];
        outEdges = edges[inSubtype];
        outFaces = faces[inSubtype];
        outWeights = weights[inSubtype];
        return true;
      }
      return false;
    }

    // Need to also modify the switch statement in selectNextFamily (or refactor)
    enum Family {
      fam_polyhedra = 0,
      fam_polychora,
      fam_duoprisms,
      fam_truncated_hedra,
      fam_truncated_chora,
      fam_bitrunc_chora,
      fam_edge_trunc_hedra,
      fam_edge_trunc_chora,
      fam_cayley_graphs,
      fam_solids,
      fam_mazes,
      fam_222_family,
      fam_227_family,
      fam_233_family,
      fam_234_family,
      fam_235_family,
      fam_Y_family,
      fam_333_family,
      fam_334_family,
      fam_343_family,
      fam_335_family,
      numFamilies,
    };

    //if it couldn't find the subtype in the family, it increments the subtype and picks that
    bool selectNextFamily(int& inFamily, int& inSubtype,
        int& outCode, int& outEdges, int& outFaces, int& outWeights) {
      outCode = the_5_cell; // default as no error handling
      // set some default values as most families have the same settings for these
      outEdges = 1111;
      outFaces = 111111;
      outWeights = 1111;

      bool found = false;
      while(!found) {
        switch (inFamily) {
          //misc full-faced families
          case  0: found = selectNextSubFamily(inSubtype, phedra_message, 5, phedra_nums, outCode);  break;
          case  1: found = selectNextSubFamily(inSubtype, pchora_message, 6, pchora_nums, outCode);  break;
          case  2: found = selectNextSubFamily(inSubtype, dprism_message, 8, dprism_nums, outCode);  break;
          case  3: found = selectNextSubFamily(inSubtype, thedra_message, 6, thedra_nums, outCode);  break;
          case  4: found = selectNextSubFamily(inSubtype, tchora_message, 8, tchora_nums, outCode);  break;
          case  5: found = selectNextSubFamily(inSubtype, bchora_message, 5, bchora_nums, outCode);  break;
          case  6: found = selectNextSubFamily(inSubtype, ehedra_message, 6, ehedra_nums, outCode);  break;
          case  7: found = selectNextSubFamily(inSubtype, echora_message, 8, echora_nums, outCode);  break;
          case  8: found = selectNextSubFamily(inSubtype, cayley_message, 12, cayley_nums, outCode); break;

          //partial-faced families
          case  9: found = selectNextSubFamily(inSubtype, solids_message, 8, solids_nums,
              solids_edges, solids_faces, solids_weights, outCode, outEdges, outFaces, outWeights);  break;
          case 10: found = selectNextSubFamily(inSubtype, mazes_message, 8, mazes_nums,
              mazes_edges, mazes_faces, mazes_weights, outCode, outEdges, outFaces, outWeights);     break;

          //quotient lattice families
          case 11: found = selectNextSubFamily(inSubtype, "2-2-2", 15, fam222_nums, outCode);  break;
          case 12: found = selectNextSubFamily(inSubtype, "2-2-7", 15, fam227_nums, outCode);  break;
          case 13: found = selectNextSubFamily(inSubtype, "2-3-3", 15, fam233_nums, outCode);  break;
          case 14: found = selectNextSubFamily(inSubtype, "2-3-4", 15, fam234_nums, outCode);  break;
          case 15: found = selectNextSubFamily(inSubtype, "2-3-5", 15, fam235_nums, outCode);  break;
          case 16: found = selectNextSubFamily(inSubtype, "    Y", 15, fam_Y__nums, outCode);  break;
          case 17: found = selectNextSubFamily(inSubtype, "3-3-3", 15, fam333_nums, outCode);  break;
          case 18: found = selectNextSubFamily(inSubtype, "3-3-4", 15, fam334_nums, outCode);  break;
          case 19: found = selectNextSubFamily(inSubtype, "3-4-3", 15, fam343_nums, outCode);  break;
          case 20: found = selectNextSubFamily(inSubtype, "3-3-5", 15, fam335_nums, outCode);  break;
          default: 
            return false;
        }

        if(found) {
          return true;
        } else {
          inSubtype = 0;
          inFamily++;
        }
      }
      //assert(false);
      return false; // shouldn't actually hit here
    }

    // ugh all this code is gross.
    // It's my fault because I am defiling a decent menu system into a static enumerator
    static int s_nextFamily = 0;
    static int s_nextSubtype = 0;

    ToddCoxeter::Graph* selectNext() {

      int code;
      int edges;
      int faces;
      int weights;
      if(selectNextFamily(s_nextFamily, s_nextSubtype, code, edges, faces, weights)) {
        s_nextSubtype++;
      } else {
        s_nextFamily = 0;
        s_nextSubtype = 0;
      }

      return select(code, edges, faces, weights);
    }

    // this doesn't exactly work perfectly, but whatever
    ToddCoxeter::Graph* selectPrev() {
      int code;
      int edges;
      int faces;
      int weights;
      if(selectNextFamily(s_nextFamily, s_nextSubtype, code, edges, faces, weights)) {
        s_nextSubtype--;
        if(s_nextSubtype < 0) {
          s_nextSubtype = 0;
          s_nextFamily--;
          if(s_nextFamily < 0) {
            s_nextFamily = Family::numFamilies - 1;
          }
        }
      } else {
        s_nextFamily = 0;
        s_nextSubtype = 0;
      }

      return select(code, edges, faces, weights);
    }

  } // namespace Polytope

} //namespace jenn