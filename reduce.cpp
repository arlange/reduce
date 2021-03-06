#include <iostream>
//#include <flens/flens.cxx>
//#include <cmath>
#include <time.h>
#include <limits>
#include <unistd.h>
#include "lll.h"
#include "/home/alex/research/archer/g.h"

using namespace std;
//using namespace flens;


void get_matrix( g * gr, GEMatrix * gr_A ){
  int n = gr->order();
  gr_A->resize(n,n);
  
  for( int i = 0; i < n-1; i++ ){
    for( int j = i+1; j < n; j++ ){
      if( gr->is_edge(i,j) ){
	(*gr_A)(i+1,j+1)=1;
	(*gr_A)(j+1,i+1)=1;
      }
    }
  }
}

void get_dom_basis( GEMatrix * gr_A, GEMatrix * gr_B, int scale = 2 ){
  int n = gr_A->numRows();
  gr_B->resize( n + n + n, n + n + 1);

  
  for( int i = 1; i <= 2*n; i++ ){
    (*gr_B)(i,i) = 1;
  }

  for( int i = 2*n + 1; i <= 3*n; i++ ){
    for( int j = 1; j <= n; j++ ){
      if( i-2*n == j ){
	(*gr_B)(i,j) = 1 * scale;
	(*gr_B)(i,2*n-j+1) = -1 * scale;
      }
      else
	(*gr_B)(i,j) = (*gr_A)(i-2*n,j) * scale;
    }
    (*gr_B)(i,n+n+1) = -1 * scale;
  }
}

bool is_dom_set( g * gr, int * dom_set ){
  int n = gr->order();
  bool dominated[gr->order()];
  
  for( int i = 0; i < n; i++ ){
    dominated[i] = false;
  }

  for( int i = 0; i < n; i++ ){
    if( dom_set[i] != 0 ){
      dominated[i] = true;
      for( int j = 0; j < n; j++ ){
	if( gr->is_edge(i,j) ){
	  dominated[j] = true;
	}
      }
    }
  }

  // for( int i = 0; i < n; i++ ) cout << dominated[i] << " ";
  //cout << endl;
  
  for( int i = 0; i < n; i++ )
    if( !dominated[i] ) return false;

  return true;
  
}

bool check_vec( GEMatrix::View gr_B, int row_size, int n ){
  /*cout << "CHECK" << endl;
    cout << col << endl;*/
  bool good_one = true;
  bool still_good_one = true;
      
  for( int r = row_size; r >= row_size-n; r-- ){
    if( gr_B(r,1) != 0 ){
      good_one = false;			
      break;
    }
  }
  if( good_one ){
    bool posi = true;
    bool found_first = false;
    for( int r = 1; r <= 2*n; r++ ){
      int val = gr_B(r,1);
      if( val != 0 ){
	if( !found_first ){
	  posi = val > 0;
	  found_first = true;
	}
	else{
	  if( (posi && val < 0) || (!posi && val > 0 ) ){
	    still_good_one = false;
	    break;
	  }
	}
      }
    }
  }
  return still_good_one && good_one;
}

GEMatrix * bt_vec;
GEMatrix * bt_grB;
int bt_depth_limit;
int bt_cur_depth;
int bt_rsize;
int bt_csize;
int bt_n;

void bt_depth_first( int e, int c ){
  //  *bt_vec = *bt_vec + e *  (*bt_grB)(_,_(c,c) );
  cout << " + " << e << " " << c << endl;
  bt_cur_depth += 1;
  // bool good = check_vec( *bt_vec, bt_rsize, bt_n );
  if( bt_cur_depth < bt_depth_limit ){
    bt_depth_first( 1, c+1 );
    cout << "end " << c << endl;
  }
  //    *bt_vec = *bt_vec - e *  (*bt_grB)(_,_(c,c) );
  cout << " - " << e << " " << c << endl;
  bt_cur_depth -= 1;
  if( e == 1 )
    bt_depth_first( -1, c  );
  else if( c < bt_csize )
    bt_depth_first( 1, c + 1  );
}



void backtrack_search( GEMatrix * gr_B, int row_size, int col_size, int n, int depth ){	
  cout << "DEPTH " << depth << endl;
  bt_grB = gr_B;
  bt_rsize = row_size;
  bt_csize = col_size;
  bt_n = n;
  bt_vec = new GEMatrix( 1, row_size );
  bt_depth_limit = depth;
  bt_cur_depth = 0;
  bt_depth_first( 1, 1 );
}


void kr_reduction( GEMatrix * gr_B, double y, bool taxi_lll, bool taxi_wr, ostream * out = &cout ){
  int col_size = gr_B->numCols();
  double delta_t[ col_size ];
  double ** delta;
  delta = new double*[ col_size ];
  for( int i = 0; i < col_size; i++ ){
    delta[i] = new double[ col_size];
  }
  
  lll( gr_B, y, taxi_lll );

  if( taxi_wr ) fill_delta_t( gr_B, delta_t );  
  else fill_delta( gr_B, delta, delta_t );

  double weight = 0;
  print_matrix(gr_B);
  for( int i = 0; i < col_size; i++ )
    weight += delta_t[i];

  *out << endl;
    

  double new_weight = 0;
  double old_weight = weight;
  int num_rounds = 0;

  

  while( new_weight < weight ){
    *out << "weight = " << weight << endl;
    *out << "new weight = " << new_weight << endl;
    *out << "old weight = " << old_weight << endl;
    num_rounds ++;
    weight = old_weight; new_weight = 0;
    if( taxi_wr ) wr_taxi( gr_B, delta_t );
    else wr( gr_B, delta );
    sort_basis( gr_B, delta_t );

    if( taxi_wr ) fill_delta_t( gr_B, delta_t );  
    else  fill_delta( gr_B, delta, delta_t );
    
    for( int i = 0; i < col_size; i++ )
      new_weight += delta_t[i];

    *out << "   weight before lll " << new_weight << endl;
    new_weight = 0;

    lll( gr_B, y, taxi_lll );

    if( taxi_wr ) fill_delta_t( gr_B, delta_t );  
    else  fill_delta( gr_B, delta, delta_t );

    for( int i = 0; i < col_size; i++ )
      new_weight += delta_t[i];
    //    
    old_weight = new_weight; 
    *out << "   weight after lll " << new_weight << endl;
  }

  int num_replaced = 1;

  while( num_replaced > 0 ){
    if( taxi_wr ) num_replaced = wr_taxi( gr_B, delta_t );
    else{
      num_replaced = wr( gr_B, delta );
      fill_delta( gr_B, delta, delta_t );
    }
    *out << "  num replaced = " << num_replaced << endl;
  }

  /* JUST WR-TAXI
     fill_delta_t( gr_B, delta_t );
  while( num_replaced > 0 ){
    num_replaced =  wr_taxi( gr_B, delta_t );
    *out << "  num replaced = " << num_replaced << endl;
  }
  fill_delta( gr_B, delta, delta_t );*/
  new_weight = 0;
  for( int i = 0; i < col_size; i++ )
  new_weight += delta_t[i];

  *out << "NUM ROUNDS " << num_rounds << endl;
  *out << "final weight = " << new_weight << endl;
  *out << endl;
  //print_matrix(gr_B);
  
  //  *out << endl;
  // print_matrix(gr_B);
}


int main( int argc, char * argv[] ){
  char opt;
  string out_file;
  int n, row_size, col_size, found, sum_found, sum3_found;
  int num_tests;
  double y = .95;
  double cur_time, avg_time, min_time, max_time;
  int sc = 10;
  bool taxi_lll = false;
  bool taxi_wr = false;
  // istream test;

  clock_t start, stop;

  opt = 'g';

  /* checks if opt is chosen
     supported options:
       k = use the KR method of LLL and WR as defined in Simple t-Designs
       d = use exponential 2^d algorithm
  */
  int s = 0;
  if( argc > 1 ){
    if( argv[1][0] == '-' ){
      opt = argv[1][1];
      argc = argc - 1;
      s = 1;
    }
  }

  if( argc == 2 ){
    out_file = argv[1+s];
  }
  else if( argc == 3 ){
    y = atof( argv[1+s] );
    out_file = argv[2+s];
  }
  else if( argc == 4 ){
    y = atof( argv[1+s] );
    sc = atoi( argv[2+s] );
    out_file = argv[3+s];
  }
  else if( argc == 5 ){
    y = atof( argv[1+s] );
    taxi_lll = atoi( argv[2+s] );
    taxi_wr = atoi( argv[3+s] );
    out_file = argv[4+s];
  }
  else if( argc == 6 ){
    y = atof( argv[1+s] );
    sc = atoi( argv[2+s] );
    taxi_lll = atoi( argv[3+s] );
    taxi_wr = atoi( argv[4+s] );
    out_file = argv[5+s];
  }
  else{
    cerr << "Error: Invalid args" << endl;
    return 1;
  }



  bool print_graphs = false;

  vector<string> graph6s;
  string g_string;

  string graph_file, log_file;

  graph_file = out_file + ".graphs";
  ofstream g_file;
  if( print_graphs ){
    g_file.open( graph_file.c_str() );
    if( !g_file.is_open() ){
      cerr << "Error opening " << graph_file << endl;
      return 0;
    }
  }

  log_file = out_file + ".log";
  ofstream log( log_file.c_str() );
  if( !log.is_open() ){
    cerr << "Error opening " << log_file << endl;
    return 0;
  }

  // Graphs from file (easier)
  if( opt == 'g' || opt == 'd' || opt == 'k' ){
    while( getline( cin, g_string ) ){
      graph6s.push_back( g_string );
    }

    num_tests = graph6s.size();
  }

  found = 0; sum_found = 0; sum3_found = 0;
  avg_time = 0; min_time = numeric_limits<double>::max( ); max_time = 0;
  bool print_more = false;

  int test_num = 1;
  for( vector<string>::iterator it = graph6s.begin();
       it != graph6s.end(); it++ ){

    // start time
    start = clock();
    
    // get graph
    int n_char = (*it)[0] - 63;
    if( n_char < 63 ){
      n =  n_char;
    }
    else{
      n = 0;
      for( int i = 1; i <=3; i++ ){
	int x = (*it)[i] - 63;
	n = n | x << (3-i)*6;

      }
    }
    g gr(n);
    gr.read_g6( *it );
    
    row_size = 3*n;
    col_size = 2*n + 1;

    // Variables for best (minimal) domination set found
    int min_dom = n+1;
    int final_dom_set[n];

    if( print_graphs ) gr.print_g6( &g_file );

    // A is adjacency matrix, B is basis for domination
    GEMatrix gr_A;
    GEMatrix gr_B;
    
    get_matrix( &gr, &gr_A );
    
    cout << gr_A << endl;
    
    get_dom_basis( &gr_A, &gr_B, sc );
    
    print_matrix( &gr_B );

    if( print_more ) print_matrix( &gr_B, &log );

    int lll_rounds;

    bool success = false; bool sum_success = false; bool sum3_success = false;
    int maxd = gr.max_degree();
    int wr_runs = 0;
    int num_replaced = 1;
    int wr_replaces = 0;

    if( opt == 'k' ){
      //      kr_reduction( &gr_B, y, taxi_lll, taxi_wr );
      kr_reduction( &gr_B, y, taxi_lll, taxi_wr, &log );
    }
    else{
      lll_rounds = lll( &gr_B, y, taxi_lll );

      cout << endl;
    
      print_matrix( &gr_B );
      if( print_more ){
	log << endl;
	print_matrix( &gr_B, &log );
      }
    
     
      log << "**** " << test_num << " ****" << endl;
      
      log << "*Max Degree = " << maxd << endl;
      cout << "Max Degree = " << maxd << endl;

      double delta_taxi[ col_size ];
      double ** delta_l2;
      delta_l2 = new double*[ col_size ];
      for( int i = 0; i < col_size; i++ ){
	delta_l2[i] = new double[ col_size];
      }

      if( taxi_wr ){
	for( int i = 1; i <= col_size; i++ ){
	  DEVector temp = gr_B(_,_(i,i) ).vectorView();
	  delta_taxi[i-1] = norm( &temp, true );
	  //      cout << delta[i-1] << endl;
	}
      }
      else{
	cout << "Filling..." << endl;
	fill_delta( &gr_B, delta_l2, delta_taxi );
	cout << "Done!" << endl;
      }

      for( int i = 0; i < col_size; i++ ){
	cout << delta_l2[i][i] << endl;
      }

      // WR

      cout << "Before WR..." << endl;
      while( num_replaced > 0 ){
	if( taxi_wr ) num_replaced = wr_taxi( &gr_B, delta_taxi );
	else{
	  num_replaced = wr( &gr_B, delta_l2 );
	  cout << num_replaced << endl;
	}
	cout << "WR" << endl;
	wr_replaces += num_replaced;
	wr_runs++;

	if( num_replaced < 0 ){
	  cerr << "ERROR WITH WR and DELTA" << endl;
	  log << "*ERROR WITH WR and DELTA" << endl;
	}
      }
      cout << "After WR..." << endl;
      log << "After WR..." << endl;
      print_matrix( &gr_B );
      if( print_more ) print_matrix( &gr_B, &log );

    }

    
    int num_vecs_found = 0; int num_dom_vecs_found = 0;
    for( int c = 1; c <= col_size && !success; c++ ){

      bool still_good_one = check_vec( gr_B(_,_(c,c) ), row_size, n );
      
      if( still_good_one ){
	//	log << test_num << " FOUND ONE, column " << c << endl;;
	cout << "FOUND ONE " << c << endl;

	num_vecs_found++;

	int dom_set[n];
	for( int r = 1; r <= n; r++ ){
	  dom_set[r-1] = gr_B(r,c);
	}
	  
	  
	// if the vector is a domination set
	if( is_dom_set( &gr, dom_set ) ){
	  success = true;
	  num_dom_vecs_found++;
	    
	  cout << "... AND it's a dominating set" << endl;
	  log << "... AND it's a dominating set:" << endl;
	  cout << "x = ";
	  log << "x = ";
	  int dom_size = 0;
	  for( int i = 0; i < n; i++ ){
	    log << dom_set[i] << " ";
	    cout << dom_set[i] << " ";
	    if( dom_set[i] != 0 )
	      dom_size++;
	  }
	  log << endl;
	  log << "v = ";
	  for( int i = 1; i <= row_size; i++ ){
	    log << gr_B(i,c) << " ";
	  }
	  log << endl;
	  log << "Cardinality: " << dom_size << endl;
	  if( dom_size < min_dom ){
	    min_dom = dom_size;
	    for( int i = 0; i < n; i++ ) final_dom_set[i] = dom_set[i];
	  }
	}
      }
    }

    if( !success ){
      if( opt == 'd' ){ // backtrack search
	cout << "Did not find one, attempting to sum columns with backtrack" <<endl;
	log << "Did not find one, attempting to sum columns with backtrack" <<endl;
	backtrack_search( &gr_B, row_size, col_size, n, maxd );
      }
      else{
	log << "Did not find one, attempting to sum columns..." <<endl;
	cout << "Did not find one, attempting to sum columns..." <<endl;
	for( int c1 = 1; c1 < col_size; c1++ ){
	  for( int c2 = c1+1; c2 <=col_size; c2++ ){

	    bool go = true;
	    int scale = 1;

	    while(go){

	      GEMatrix sum = gr_B(_,_(c1,c1) ) + scale * gr_B(_,_(c2,c2) );
	      bool still_good_one = check_vec( sum, row_size, n );
      
	      if( still_good_one ){
		num_vecs_found++;
		int dom_set[n];
		for( int r = 1; r <= n; r++ ){
		  dom_set[r-1] = sum(r,1);
		}
	    
		cout << "FOUND ONE " << c1 << " + " << c2 << endl;
	  
		// if the vector is a domination set
		if( is_dom_set( &gr, dom_set ) ){
		  sum_success = true;
		  num_dom_vecs_found++;
		
		  cout << "... AND it's a dominating set" << endl;
		  log << "FOUND ONE " << c1 << " + " << scale << "x" << c2 << endl;
		  log << "... AND it's a dominating set:" << endl;
		  cout << "x = ";
		  log << "x = ";
		  int dom_size = 0;
		  for( int i = 0; i < n; i++ ){
		    log << dom_set[i] << " ";
		    cout << dom_set[i] << " ";
		    if( dom_set[i] != 0 )
		      dom_size++;
		  }
		  log << endl;
		  log << "v = ";
		  for( int i = 1; i <= row_size; i++ ){
		    log << sum(i,1) << " ";
		  }
		  log << endl; cout << endl;
		  log << "Cardinality: " << dom_size << endl;
		  if( dom_size < min_dom ){
		    min_dom = dom_size;
		    for( int i = 0; i < n; i++ ) final_dom_set[i] = dom_set[i];
		  }
		
		}
	      }
	      if( scale == -1 )
		go = false;
	      else
		scale = -1;
	    }
	  }
	}
      

      if( !success && !sum_success ){
	log << "Did not find with sum, attempting to sum three columns..." <<endl;
	cout << "Did not find with sum, attempting to sum three columns..." <<endl;
	for( int c1 = 1; c1 < col_size-1; c1++ ){
	  for( int c2 = c1+1; c2 < col_size; c2++ ){
	    for( int c3 = c2+1; c3 <= col_size; c3++ ){
	      bool go = true;
	      int scale1 = 1;
	      int scale2 = 1;
	      while( go ){
		GEMatrix sum = gr_B(_,_(c1,c1) ) + scale1 * gr_B(_,_(c2,c2) )
		  + scale2 * gr_B(_,_(c3,c3) );
		bool still_good_one = check_vec( sum, row_size, n );
	      
		if( still_good_one ){
		  cout<< test_num << " FOUND ONE, columns "  << c1
		      << " + " << scale1 << " x" << c2
		      << " + " << scale2 << " x" << c3 << endl;
		  
		  num_vecs_found++;
		  int dom_set[n];
		  for( int r = 1; r <= n; r++ ){
		    dom_set[r-1] = sum(r,1);
		  }
	    
		  if( is_dom_set( &gr, dom_set )  ){
		    sum3_success = true;
		    num_dom_vecs_found++;
		    log << test_num << " FOUND ONE, columns "  << c1
			<< " + " << scale1 << " x" << c2
			<< " + " << scale2 << " x" << c3 << endl;
		    cout << "... AND it's a dominating set" << endl;
		    log << "... AND it's a dominating set:" << endl;
		    cout << "x = ";
		    log << "x = ";
		    int dom_size = 0;
		    for( int i = 0; i < n; i++ ){
		      log << dom_set[i] << " ";
		      cout << dom_set[i] << " ";
		      if( dom_set[i] != 0 )
			dom_size++;
		    }
		    log << endl;
		    log << "v = ";
		    for( int i = 1; i <= row_size; i++ ){
		      log << sum(i,1) << " ";
		    }
		    log << endl; cout << endl;
		    log << "Cardinality: " << dom_size << endl;
		    if( dom_size < min_dom ){
		      min_dom = dom_size;
		      for( int i = 0; i < n; i++ ) final_dom_set[i] = dom_set[i];
		    }
		  }
		}
		if( scale1 == -1 && scale2 == -1)
		  go = false;
		else if( scale1 == -1 ){
		  scale2 = -1;
		  scale1 = 1;
		}
		else if( scale2 == -1 ){
		  scale1 = -1;
		}
		else if( scale1 == 1 ){
		  scale1 = -1;
		}
	      }
	    }
	  }
	}
      }
        print_matrix(&gr_B);
      }
    }

    bool good = success || sum_success || sum3_success;

    // time stuff
    stop = clock();
    cur_time = ((float)(stop - start))/((float)CLOCKS_PER_SEC);
    avg_time += cur_time;
    if( cur_time < min_time ) min_time = cur_time;
    if( cur_time > max_time ) max_time = cur_time;

    int num_summed_needed = -1;
    if( success ){ 
      found++;
      num_summed_needed = 1;
    }
    else if( sum_success ){
      sum_found++;
      num_summed_needed = 2;
    }
    else if( sum3_success ){
      sum3_found++;
      num_summed_needed = 3;
    }

    
    log << "*Order = " << n << endl;
    log << "*Basis is " << row_size << " x " << col_size << endl;
    if( good ){
      log << "*Dominating set FOUND, number = " << min_dom << "\n*x = ";
      for( int i = 0; i < n; i++ ) log << final_dom_set[i] << " ";
      log << endl;
    }
    else log << "*Dominating set not found" << endl;
    log << "*Number of LLL loops: " <<  lll_rounds << endl;
    log << "*Number of WR runs: " << wr_runs << endl;
    log << "*Number of WR replaces: " << wr_replaces << endl;
    log << "*Number of valid vecs found: " << num_vecs_found << endl;
    log << "*Number of Dominating vecs found: " << num_dom_vecs_found << endl;
    log << "*Time = " << cur_time << endl;
    log << "*Found at stage ";
    if( num_summed_needed > 0 ) log << num_summed_needed << endl;
    else log << "NONE" << endl;
    log << "^" << y << " & ";
    if( good ) log << "Y " << min_dom << " & " << num_summed_needed;
    else log << "N & n/a & n/a";
    log << " & " << lll_rounds << " & " << wr_runs << " & " << wr_replaces << " & "
	<< num_vecs_found << " & " << num_dom_vecs_found  
	<< " & " << cur_time << endl;
    test_num++;
    log << endl;

  }
  avg_time = avg_time / num_tests;
  log << "******************" << endl;
  log << "**y = " << y << ", " << "scale = " << sc << endl;
  log << "**LLL Taxi " << ( taxi_lll ? "TRUE" : "FALSE" )
      << ", WR Taxi " << (  taxi_wr ? "TRUE" : "FALSE" ) << endl;
  log << "**Time: " << "avg = " << avg_time << ", min = " << min_time 
      << ", max = " << max_time << endl;
  log << "**Original found: " << found << endl;
  log << "**Sum2 found: " << sum_found << endl;
  log << "**Sum3 found: " << sum3_found << endl;
  int total = found + sum_found + sum3_found;
  log << "**Total found: " << total << " / " << num_tests << endl;
  log << "^^" << found << " & " << sum_found << " & " << sum3_found
      << " & " << total << " & " << avg_time << " & " << min_time 
      << " & " << max_time << endl;
  if( print_graphs ) g_file.close();
  log.close();
  cout << endl;
}




void test( ){

  GEMatrix B(4,4);
  GEMatrix Bstar(4,4);

  DEVector x(3), y(3);

  B = 1, 2, 3,  4,
      5, 6, 7,  8,
      9, 8, 7,  6,
      5, 4, 3, 20;

  cout << B << endl;
  cout << endl;

  /*  GEMatrix alpha = gram_schmidt( &B, &Bstar );

  cout << Bstar << endl;
  cout << endl;
  cout << alpha << endl;

  blas::swap( Bstar(_,_(1,1)).vectorView(), Bstar(_,_(2,2)).vectorView() );
  cout << endl;
  cout << Bstar << endl; */

}


/* OLD WAY OF DOING IT WITH OPT

  // Hamming graph
  else if( opt == 'h' ){
    cout << "Testing LLL domination with a Hamming graph" << endl;
    int l;
    if( argc == 4 ){
      l = atoi( argv[3] );
    }
    else{
      cout << "l? ";
      cin >> l;
    }
    n = pow( 3, l );
    g ham(n);
    ham.make_hamming( l );
    graph6s.push_back( ham.to_g6() );

  }
  // Generate random Erdos-Renyi graphs
  else if( opt == 'r' ){
    float p;
        
    cout << "Testing LLL domination with random graphs..." << endl;
    cout << "n? ";
    cin >> n; 
    cout << "p? ";
    cin >> p;
    cout << "Number of tests? ";
    cin >> num_tests;
    //    cout << "Filename? ";
    //cin >> out_file;

    log << "Random graphs" << endl;
    log << n << " " << p << " " << num_tests << endl;
    

    cout << "Generating random graphs..." << endl;
    for( int i = 0; i < num_tests; i++ ){
      g gr( n );
      int added = gr.make_rand_er( p );
      graph6s.push_back( gr.to_g6() );
      sleep(2);
    }
  }*/
