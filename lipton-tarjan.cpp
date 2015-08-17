#include "lipton-tarjan.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <csignal>
#include <boost/lexical_cast.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/planar_canonical_ordering.hpp>
#include <boost/graph/is_straight_line_drawing.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp> 
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/make_biconnected_planar.hpp>
#include <boost/graph/make_maximal_planar.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/range/irange.hpp> 
using namespace std;
using namespace boost; 

int levi_civita(uint i, uint j, uint k)
{
        if( i == j || j == k || k == i ) return 0; 
        if( i == 1 && j == 2 && k == 3 ) return 1;
        if( i == 2 && j == 3 && k == 1 ) return 1;
        if( i == 3 && j == 1 && k == 2 ) return 1;
        return -1;
} 

ostream& operator<<(ostream& o, VertDesc v)
{
        o << vert2uint[v];
        return o;
}

string to_string(EdgeDesc e, Graph const& g)
{
        auto src = lexical_cast<string>(vert2uint[source(e, g)]);
        auto tar = lexical_cast<string>(vert2uint[target(e, g)]);
        return src + ", " + tar;
}

bool on_cycle(VertDesc v, vector<VertDesc> const& cycle, Graph const& g)
{
        return find(cycle.begin(), cycle.end(), v) != cycle.end();
}

bool on_cycle(EdgeDesc e, vector<VertDesc> const& cycle, Graph const& g)
{
        auto src = source(e, g);
        auto tar = target(e, g);
        return on_cycle(src, cycle, g) && on_cycle(tar, cycle, g);
}

bool edge_inside(EdgeDesc e, VertDesc v, vector<VertDesc> const& cycle, Graph const& g, Embedding& em)
{
        cout << "        testing if edge " << to_string(e, g) << " is inside the cycle: ";
        auto it     = find(cycle.begin(), cycle.end(), v);
        auto before = it   == cycle.begin() ?  cycle.end  ()-1   : it-1;
        auto after  = it+1 == cycle.end  () ?  cycle.begin()     : it+1; 
        auto other  = (source(e, g) == v) ?  target(e, g)        : source(e, g); 

        vector<uint> perm;
        for( auto& tar_it : em[*it] ){
                auto src = source(tar_it, g);
                auto tar = target(tar_it, g);
                if(src != v ) swap(src, tar);
                assert(src == v);

                if( tar == other   ) perm.push_back(1);
                if( tar == *before ) perm.push_back(2);
                if( tar == *after  ) perm.push_back(3);
        }
        assert(perm.size() == 3);
        if( levi_civita(perm[0], perm[1], perm[2]) == 1 ){
                cout << "YES\n";
                return true;
        } else {
                cout << "NO\n";
                return false;
        }
}


struct BFSVert
{
        BFSVert() : parent(Graph::null_vertex()), level(0), cost(0) {}

        VertDesc parent;
        int      level;
        uint     cost;
};

struct BFSVisitorData
{
        map<VertDesc, set<VertDesc>> children;
        map<VertDesc, BFSVert>       verts;
        int                          num_levels;
        Graph*                       g;
        VertDesc                     root;

        BFSVisitorData(Graph* g) : g(g), num_levels(0), root(Graph::null_vertex()) {}

        void reset(Graph* g)
        {
                children.clear();
                verts   .clear();
                num_levels = 0;
                this->g = g;
                root = Graph::null_vertex();
        }
        

        bool is_tree_edge(EdgeDesc e)
        { 
                auto src = source(e, *g);
                auto tar = target(e, *g); 
                return verts[src].parent == tar || verts[tar].parent == src;
        }

        uint edge_cost(EdgeDesc e, vector<VertDesc> const& cycle, Graph const& g)
        {
                //cout << "++++++++ computing edge cost ++++++++\n";
                assert(is_tree_edge(e));

                auto v = source(e, g); 
                auto w = target(e, g); 

                if( !on_cycle(v, cycle, g) ) swap(v, w);

                assert( on_cycle(v, cycle, g));
                assert(!on_cycle(w, cycle, g));
                //cout << "on cycle:  " << v << '\n';
                //cout << "off cycle: " << w << '\n';

                uint total = num_vertices(g);
                //cout << "total: " << total << '\n';

                int cost;
                if( verts[w].parent == v ){
                        cost = verts[w].cost;
                        //cout << "v is parent\n";
                } else {
                        assert(verts[v].parent == w);
                        cost = total - verts[v].cost;
                        //cout << "v is NOT parent\n";
                        //cout << "v cost: " << verts[v].cost << '\n';
                }
                //cout << "cost: " << cost << '\n';
                //cout << "-------------------------------------\n";
                return cost;
        } 

        void print_costs()
        {
                for( auto& v : verts ) cout << "cost of vertex " << v.first << " is " << v.second.cost << '\n';
        }
};

struct BFSVisitor : public default_bfs_visitor
{
        BFSVisitorData& data;

        BFSVisitor(BFSVisitorData& data) : data(data) {}

        template<typename Edge, typename Graph> void tree_edge(Edge e, Graph const& g)
        {
                auto parent = source(e, g);
                auto child  = target(e, g);
                cout << "  tree edge " << parent << ", " << child;
                data.verts[child].parent = parent;
                data.verts[child].level  = data.verts[parent].level + 1;
                data.num_levels = max(data.num_levels, data.verts[child].level + 1);
                if( Graph::null_vertex() != parent ) data.children[parent].insert(child);

                VertDesc v = child;
                data.verts[v].cost = 1;
                cout << "     vertex/cost: ";
                cout << v << '/'  << data.verts[v].cost << "   ";
                while( data.verts[v].level ){
                        v =  data.verts[v].parent;
                        ++data.verts[v].cost;
                        cout << v << '/'  << data.verts[v].cost << "   ";
                } 
                cout << '\n';
        } 
};

extern void print_graph(Graph const& g);

uint theorem4(uint partition, Graph const& g)
{
        /*
        Assume G is connected.
        Partition the vertices into levels according to their distance from some vertex v.
        L[l] = # of vertices on level l
        If r is the maximum distance of any vertex from v, define additional levels -1 and r+1 containing no vertices
        l1 = the level such that the sum of costs in levels 0 thru l1-1 < 1/2, but the sum of costs in levels 0 thru l1 is >= 1/2
        (If no such l1 exists, the total cost of all vertices < 1/2, and B = C = {} and return true)
        k = # of vertices on levels 0 thru l1.
        Find a level l0 such that l0 <= l1 and |L[l0]| + 2(l1-l0) <= 2sqrt(k)
        Find a level l2 such that l1+1 <= l2 and |L[l2] + 2(l2-l1-1) <= 2sqrt(n-k)
        If 2 such levels exist, then by Lemma 3 the vertices of G can be partitioned into three sets A, B, C such that no edge joins a vertex in A with a vertex in B,
        neither A or C has cost > 2/3, and C contains no more than 2(sqrt(k) + sqrt(n-k)) vertices.
        But 2(sqrt(k) + sqrt(n-k) <= 2(sqrt(n/2) + sqrt(n/2)) = 2sqrt(2)sqrt(n)
        Thus the theorem holds if suitable levels l0 and l2 exist
                Suppose a suitable level l0 does not exist.  Then, for i <= l1, L[i] >= 2sqrt(k) - 2(l1-i)
                Since L[0] = 1, this means 1 >= 2sqrt(k) - 2l1 and l1 + 1/2 >= sqrt(k).  Thus l1 = floor(l1 + 1/2) > 
                Contradiction

        Now suppose G is not connected
        Let G1, G2, ... , Gk be the connected components of G, with vertex sets V1, V2, ... , Vk respectively.
        If no connected component has total vertex cost > 1/3, let i be the minimum index such that the total cost of V1 U V2 U ... U Vi > 1/3
        A = V1 U V2 U ... U Vi
        B = Vi+1 U Vi+2 U ... U Vk
        C = {}
        Since i is minimum and the cost of Vi <= 1/3, the cost of A <= 2/3. return true;
        If some connected component (say Gi) has total vertex cost between 1/3 and 2/3,
        A = Vi
        B = V1 U ... U Vi-1 U Vi+1 U ... U Vk
        C = {}
        return true

        Finally, if some connected component (say Gi) has total vertex cost exceeding 2/3,
        apply the above argument to Gi
        Let A*, B*, C* be the resulting partition.
        A = set among A* and B* with greater cost
        C = C*
        B = remanining vertices of G
        Then A and B have cost <= 2/3, g
        return true;

        In all cases the separator C is either empty or contained in only one connected component of G
        */
        return partition;
}

struct ScanVisitor
{
        map<VertDesc, bool>*          table;
        Graph*                        g;
        VertDesc                      x;
        int                           l0;
        set<pair<VertDesc, VertDesc>> edges_to_add, edges_to_delete;

        ScanVisitor(map<VertDesc, bool>* table, Graph* g, VertDesc x, int l0) : table(table), g(g), x(x), l0(l0) {}

        void foundedge(VertDesc V, EdgeDesc e)
        {
                auto v = source(e, *g);
                auto w = target(e, *g);
                if( V != v ) swap(v, w);
                assert(V == v);
                cout << "foundedge " << v << ", " << w << '\n';
                if ( !(*table)[w] ){
                        (*table)[w] = true;
                        assert(x != w); 
                        cout << "   !!!!!!!going to add " << x << ", " << w << '\n';
                        edges_to_add.insert(make_pair(x, w));
                }
                cout << "     going to delete " << v << ", " << w << '\n';
                edges_to_delete.insert(make_pair(v, w)); 
        }

        void finish()
        {
                cout << "finishing\n";
                cout << "edges to add size: " << edges_to_add.size() << '\n';
                cout << "adding: ";
                for( auto& p : edges_to_add    ){
                        assert(p.first != p.second);
                        cout << '(' << p.first << ", " << p.second << ")   ";
                        add_edge(p.first, p.second, *g);
                }
                cout << '\n';
                cout << "edges to remove size: " << edges_to_delete.size() << '\n';
                cout << "removing: ";
                for( auto& p : edges_to_delete ){ 
                        cout << '(' << p.first << ", " << p.second << ")   ";
                        remove_edge(p.first, p.second, *g);
                }
                cout << '\n';
        }
};

void scan_nonsubtree_edges(VertDesc v, Graph const& g, Embedding& em, BFSVisitorData& bfs, ScanVisitor& vis)
{
        if( bfs.verts[v].level > vis.l0 ) return;
        for( auto e : em[v] ){
                auto src = source(e, g);
                auto tar = target(e, g);
                if( src == tar ) continue; // ?????
                if( !bfs.is_tree_edge(e) ){
                        vis.foundedge(v, e);
                        continue;
                }
                if( src != v ) swap(src, tar);
                assert(src == v); 
                if( bfs.verts[tar].level > vis.l0 ) vis.foundedge(v, e);
        }
        for( auto c : bfs.children[v] ) scan_nonsubtree_edges(c, g, em, bfs, vis);
}

void reset_vertex_indices(Graph& g)
{
        VertIter vi, vend;
        uint i = 0;
        for( tie(vi, vend) = vertices(g); vi != vend; ++vi, ++i ) put(vertex_index, g, *vi, i); 
}

bool is_planar(Graph& g)
{
        reset_vertex_indices(g);
        EmbeddingStorage storage{num_vertices(g)};
        Embedding        em(storage.begin()); 
        return boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g, boyer_myrvold_params::embedding = em);
}

EdgeIndex reset_edge_index(Graph const& g)
{
        EdgeIndex edgedesc_to_uint; 
        EdgesSizeType num_edges = 0;
        EdgeIter ei, ei_end;
        for( tie(ei, ei_end) = edges(g); ei != ei_end; ++ei ) edgedesc_to_uint[*ei] = num_edges++;
        return edgedesc_to_uint;
} 

void makemaxplanar(Graph& g)
{ 
        auto index = reset_edge_index(g);
        EmbeddingStorage storage(num_vertices(g));
        Embedding        em(storage.begin());
        boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g, boyer_myrvold_params::embedding = em);
        make_biconnected_planar(g, em, index);

        reset_edge_index(g);
        boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g, boyer_myrvold_params::embedding = em);

        make_maximal_planar(g, em);

        reset_edge_index(g);
        bool planar = boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g, boyer_myrvold_params::embedding = em);
        assert(planar);
} 

void print_cycle(vector<VertDesc> const& cycle)
{
        cout << "cycle verts: ";
        for( auto& v : cycle ) cout << v << ' ';
        cout << '\n';
}

uint lemma3(vector<VertDesc> const& cycle_verts, int* l, Graph const& g)
{
        /*
        if( l[1] > l[2] ){
                cout << "A = all verts on levels 0    thru l1-1";
                cout << "B = all verts on levels l1+1 thru r";
                cout << "C = all verts on llevel l1";
        }
                
        if( l[1] < l[2] ){
                cout << "don't know\n";
                vector<VertDesc> zero_one, middle_part, one_two;
                VertIterator vei, vend;
                for( tie(vei, vend) = vertices(g); vei != vend; ++vei ){ 
                        auto v = *vei;
                        if( bfs_vertex_data[v].level <= l[1] ){ 
                                cout << "first part: " << v << '\n';
                                zero_one.push_back(v);
                                continue;
                        }
                        if( bfs_vertex_data[v].level >= l[1]+1 && 
                            bfs_vertex_data[v].level <= l[2]-1 ){
                                cout << "middle part: " << v << '\n';
                                middle_part.push_back(v);
                                continue;
                        }
                        if( bfs_vertex_data[v].level >= l[2] ){
                                cout << "last part: " << v << '\n';
                                one_two.push_back(v);
                                continue;
                        }
                        cout << "level: " << bfs_vertex_data[v].level << '\n';
                        assert(0);
                }


                //delete verts in levels l1 and l2
                        this separates remaining vertices into 3 parts: (all of which may be empty)
                                verts on levels 0 thru l1-1
                                verts on level l1+1 thru l2-1
                                verts on levels l2+1 and above
                        the only part which can have cost > 2/3 is the middle part
                        if( middle_part.size() <= 2*num_vertices(g)/3 ){
                                cout << "A = most costly part of the 3\n";
                                cout << "B = remaining 2 parts\n";
                                cout << "C = "; for( auto& v : one_two ) cout << v << ' '; cout << '\n';
                        } else {
                                delete all verts on level l2 and above
                                shrink all verts on levels l1 and belowe to a single vertex of cost zero
                                The new graph has a spanning tree radius of l2 - l1 -1 whose root corresponds to vertices on levels l1 and below in the original graph
                                Apply Lemma 2 to the new graph, A* B* C*
                                cout << "A = set among A* and B* with greater cost\n";
                                cout << "C = verts on levels l1 and l2 in the original graph plus verts in C* minus the root\n";
                                cout << "B = remaining verts\n";
                                By Lemma 2, A has total cost <= 2/3
                                But A U C* has total cost >= 1/3, so B also has total cost <= 2/3
                                Futhermore, C contains no more than L[l1] + L[l2] + 2(l2 - l1 - 1)
                        }
        }
        */
        return 0;
}

vector<VertDesc> ancestors(VertDesc v, BFSVisitorData& vis)
{
        vector<VertDesc> ans = {v};
        while( v != vis.root ){
                v = vis.verts[v].parent;
                ans.push_back(v);
        }
        return ans;
}

VertDesc common_ancestor(vector<VertDesc> const& ancestors_v, vector<VertDesc> const& ancestors_w, BFSVisitorData& vis)
{
        uint i, j;
        for( i = 0; i < ancestors_v.size(); ++i ) for( j = 0; j < ancestors_w.size(); ++j ) if( ancestors_v[i] == ancestors_w[j] ) return ancestors_v[i];
        assert(0);
        return ancestors_v[i];
}

vector<VertDesc> get_cycle(VertDesc v, VertDesc w, VertDesc ancestor, BFSVisitorData& vis_data)
{
        vector<VertDesc> cycle, tmp;
        VertDesc cur;
        cur = v;
        while( cur != ancestor ){
                cycle.push_back(cur);
                cur = vis_data.verts[cur].parent;
        }
        cycle.push_back(ancestor);

        cur = w;
        while( cur != ancestor ){
                tmp  .push_back(cur);
                cur = vis_data.verts[cur].parent;
        }
        reverse(tmp.begin(), tmp.end());
        cycle.insert(cycle.end(), tmp.begin(), tmp.end());
        return cycle;
}

Partition lipton_tarjan(Graph& g)
{
        cout << "---------------------------- 1 - Check Planarity  ------------\n";
        bool planar = is_planar(g);
        assert(planar);
        cout << "planar ok\n";

        cout << "---------------------------- 2 - Connected Components -------- \n";
        VertDescMap idx; 
        associative_property_map<VertDescMap> vertid_to_component(idx);
        VertIter vi, vj;
        tie(vi, vj) = vertices(g);
        for( uint i = 0; vi != vj; ++vi, ++i ) put(vertid_to_component, *vi, i);
        uint components = connected_components(g, vertid_to_component);
        assert(components == 1); 

        cout << "# of components: " << components << '\n';
        vector<uint> verts_per_comp(components, 0);
        for( tie(vi, vj) = vertices(g); vi != vj; ++vi ) ++verts_per_comp[vertid_to_component[*vi]]; 
        bool too_big = false;
        for( uint i = 0; i < components; ++i ) if( 3*verts_per_comp[i] > 2*num_vertices(g) ){ too_big = true; break; }
        if( !too_big ){ theorem4(0, g); return {};}

        cout << "---------------------------- 3 - BFS and Levels ------------\n";
        BFSVisitorData vis_data(&g);
        auto root = *vertices(g).first;
        vis_data.root = root;
        breadth_first_search(g, root, visitor(BFSVisitor(vis_data)));

        vector<uint> L(vis_data.num_levels + 1, 0);
        for( auto& d : vis_data.verts ) ++L[d.second.level];

        for( tie(vi, vj) = vertices(g); vi != vj; ++vi ) cout << "level/cost of vert " << *vi << ": " << vis_data.verts[*vi].level << '\n';
        for( uint i = 0; i < L.size(); ++i ) cout << "L[" << i << "]: " << L[i] << '\n';

        cout << "---------------------------- 4 - l1 and k  ------------\n";
        uint k = L[0]; 
        int l[3];
        l[1] = 0;
        while( k <= num_vertices(g)/2 ) k += L[++l[1]];
        cout << "k:  " << k << "      # of verts in levels 0 thru l1\n";
        cout << "l1: " << l[1] << "      total cost of levels 0 thru l1 barely exceeds 1/2\n";

        cout << "---------------------------- 5 - Find More Levels -------\n";
        float sq  = 2 * sqrt(k); 
        float snk = 2 * sqrt(num_vertices(g) - k); 
        cout << "sq:    " << sq << '\n';
        cout << "snk:   " << snk << '\n';

        l[0] = l[1]; for( ;; ){ float val = L.at(l[0]) + 2*(l[1] - l[0]); if( val <= sq  ) break; --l[0]; } 
        cout << "l0: " << l[0] << "     highest level <= l1\n"; 
        l[2] = l[1] + 1; for( ;; ){ float val = L.at(l[2]) + 2*(l[2] - l[1] - 1); if( val <= snk ) break; ++l[2]; } 
        cout << "l2: " << l[2] << "     lowest  level >= l1 + 1\n";

        cout << "---------------------------- 6 - Shrinktree -------------\n";
        cout << "n: " << num_vertices(g) << '\n'; 

        tie(vi, vj) = vertices(g); 
        for( VertIter next = vi; vi != vj; vi = next){
                ++next;
                if( vis_data.verts[*vi].level >= l[2] ){
                        auto i = vert2uint[*vi];
                        cout << "deleting vertex " << *vi << " of level l2 " << vis_data.verts[*vi].level << " >= " << l[2] << '\n';
                        uint2vert.erase(i);
                        vert2uint.erase(*vi);
                        clear_vertex(*vi, g);
                        remove_vertex(*vi, g);
                }
        }

        auto x = add_vertex(g); uint2vert[vert2uint[x] = 999999] = x; 
        map<VertDesc, bool> t;
        for( tie(vi, vj) = vertices(g); vi != vj; ++vi ){
                t[*vi] = (vis_data.verts[*vi].level <= l[0]);
                cout << "vertex " << *vi << " at level " << vis_data.verts[*vi].level << " is " << (t[*vi] ? "TRUE" : "FALSE") << '\n';
        }

        reset_vertex_indices(g);
        reset_edge_index(g);
        EmbeddingStorage storage{num_vertices(g)};
        Embedding        em(storage.begin());

        planar = boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g, boyer_myrvold_params::embedding = em);
        assert(planar);

        ScanVisitor svis(&t, &g, x, l[0]);
        scan_nonsubtree_edges(*vertices(g).first, g, em, vis_data, svis);
        svis.finish();

        VertDesc x_gone = Graph::null_vertex();
        if( !degree(x, g) ){
                cout << "no edges to x found, deleting\n";
                auto i = vert2uint[x];
                uint2vert.erase(i);
                vert2uint.erase(*vi);
                remove_vertex(x, g);
                x_gone = *vertices(g).first;
                cout << "x_gone: " << x_gone << '\n';
        }

        cout << "-------------------- 7 - New BFS and Make Max Planar -----\n"; 
        reset_vertex_indices(g);
        reset_edge_index(g);
        vis_data.reset(&g);
        vis_data.root = (x_gone != Graph::null_vertex()) ? x_gone : x;
        vis_data.verts[vis_data.root].cost++;

        cout << "root: " << vis_data.root << '\n'; 

        breadth_first_search(g, x_gone != Graph::null_vertex() ? x_gone: x, visitor(BFSVisitor(vis_data))); 
        makemaxplanar(g);
        reset_edge_index(g);

        cout << "----------------------- 8 - Locate Cycle -----------------\n";
        print_graph(g);
        EdgeIter ei, ei_end;
        for( tie(ei, ei_end) = edges(g); ei != ei_end; ++ei ){
                auto src = source(*ei, g);
                auto tar = target(*ei, g);
                bool exists = edge(src, tar, g).second;
                assert(exists); 
                assert(src != tar); 
                if( !vis_data.is_tree_edge(*ei) ) break;
        }
        assert(ei != ei_end);
        assert(!vis_data.is_tree_edge(*ei));
        EdgeDesc chosen_edge = *ei;
        cout << "arbitrarily choosing nontree edge: " << to_string(chosen_edge, g) << '\n';

        auto v1 = source(chosen_edge, g);
        auto w1 = target(chosen_edge, g);
        vector<VertDesc> parents_v = {v1}, parents_w = {w1};

        auto p_v = v1; while( p_v != vis_data.root ){ p_v = vis_data.verts[p_v].parent; parents_v.push_back(p_v); } 
        auto p_w = w1; while( p_w != vis_data.root ){ p_w = vis_data.verts[p_w].parent; parents_w.push_back(p_w); }

        uint i, j;
        for( i = 0; i < parents_v.size(); ++i ) for( j = 0; j < parents_w.size(); ++j ) if( parents_v[i] == parents_w[j] ) goto done;
done:
        assert(parents_v[i] == parents_w[j]);
        auto ancestor = parents_v[i];
        cout << "common ancestor: " << ancestor << '\n'; 
        auto cycle = get_cycle(v1, w1, ancestor, vis_data);

        print_cycle(cycle);

        EmbeddingStorage storage2(num_vertices(g));
        Embedding        em2(storage2.begin());
        planar = boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g, boyer_myrvold_params::embedding = em2);
        assert(planar); 

        uint cost_inside  = 0;
        uint cost_outside = 0;
        bool cost_swapped = false;

        for( auto& v : cycle ){
                cout << "   scanning cycle vert " << v << '\n';
                auto e = out_edges(v, g);
                while( e.first != e.second ){
                        if( vis_data.is_tree_edge(*e.first) && !on_cycle(*e.first, cycle, g) ){
                                uint cost = vis_data.edge_cost(*e.first, cycle, g);
                                cout << "      scanning incident tree edge " << to_string(*e.first, g) << "   cost: " << cost << '\n';
                                bool inside = edge_inside(*e.first, v, cycle, g, em2);
                                (inside ? cost_inside : cost_outside) += cost;
                        }
                        ++e.first;
                }
        }

        if( cost_outside > cost_inside ){
                swap(cost_outside, cost_inside);
                cost_swapped = true;
                cout << "!!!!!! cost swapped !!!!!!!!\n";
        }
        cout << "total inside cost:  " << cost_inside << '\n'; 
        cout << "total outside cost: " << cost_outside << '\n'; 


        cout << "---------------------------- 9 - Improve Separator -----------\n";
        auto chosen_vi = source(chosen_edge, g);
        auto chosen_wi = target(chosen_edge, g);
        assert(!vis_data.is_tree_edge(chosen_edge));
        EdgeDesc next_edge;
        while( cost_inside > num_vertices(g)*2./3 ){
                cout << "looking for a better cycle\n";

                // Locate the triangle (chosen_vi, y, chosen_wi) which has (chosen_vi, chosen_wi) as a boundary edge and lies inside the (chosen_vi, chosen_wi) cycle.
                set<VertDesc> neighbors_v, neighbors_w, intersect;

                OutEdgeIter e_cur, e_end;
                for( tie(e_cur, e_end) = out_edges(chosen_vi, g); e_cur != e_end; ++e_cur ){ auto vv = target(*e_cur, g); neighbors_v.insert(vv); cout << "vertex " << chosen_vi << "has neighbor " << vv << '\n'; }
                for( tie(e_cur, e_end) = out_edges(chosen_wi, g); e_cur != e_end; ++e_cur ){ auto vv = target(*e_cur, g); neighbors_w.insert(vv); cout << "vertex " << chosen_wi << "has neighbor " << vv << '\n'; } 
                set_intersection(neighbors_v.begin(), neighbors_v.end(), neighbors_w.begin(), neighbors_w.end(), inserter(intersect, intersect.begin()));
                for( auto& a : intersect ) cout << "set intersection: " << a << '\n'; 
                assert(intersect.size() == 2);

                VertDesc y;

                // there are 2 triangles with edge (chosen_vi, chosen_wi), one inside and one outside
                y = edge_inside(chosen_edge, *intersect.begin(), cycle, g, em2) ?
                    *intersect.begin()                                          :
                    *(++intersect.begin());

                EdgeDesc viy, ywi;
                if ( vis_data.is_tree_edge(viy) || vis_data.is_tree_edge(ywi) ){
                        next_edge = vis_data.is_tree_edge(viy) ? ywi : viy;
                        assert(!vis_data.is_tree_edge(next_edge));
                        // Compute the cost inside the (vi+1, wi+1) cycle from the cost inside the (vi, wi) cycle and the cost of vi, y, and wi.
                } else {
                        // Determine the tree path from y to the (vi, wi) cycle by following parent pointers from y.
                        auto ancestors_v = ancestors(chosen_vi, vis_data);
                        auto ancestors_w = ancestors(chosen_wi, vis_data);
                        VertDesc z = common_ancestor(ancestors_v, ancestors_w, vis_data); // the (vi, wi) cycle reached during this search.
                        
                        auto new_cycle = get_cycle(chosen_vi, chosen_wi, z, vis_data);

                        // Compute the total cost of all vertices except z on this tree path.
                                // Scan the tree edges inside the (y, wi) cycle, alternately scanning an edge in one cycle and an edge in the other cycle.
                                // Stop scanning when all edges inside one of the cycles have been scanned.
                        // Compute the cost inside this cycle by summing the associated costs of all scanned edges.
                        // Use this cost, the cost inside the (vi, wi) cycle, and the cost on the tree path from y to zy to compute the cost inside the other cycle.
                        // Let (vi+1, w+1) be the edge among (vi, y) and (y, wi) whose cycle has more cost inside it.
                }

        }
        cout << "found cycle with inside cost < 2/3: " << cost_inside << '\n';

        cout << "\n------------ 10  - Construct Vertex Partition --------------\n";
        uint partition = lemma3(cycle, &l[0], g);
        partition = theorem4(partition, g); 

        return {};
}
