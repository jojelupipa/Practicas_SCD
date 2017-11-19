#include <iostream>
#include <iomanip>
#include <chrono>  // incluye now, time\_point, duration
#include <future>
#include <vector>
#include <cmath>

using namespace std ;
using namespace std::chrono;

const int MUESTRAS = 9699690; // Mínimo común múltiplo de todos los
			  // números primos hasta el 19 para
			  // asegurarnos que sea múltiplo del número
			  // de hebras que elijamos.

int hebras; // Hebras usadas, se define en tiempo de ejecución como
	    // argumento, global para cumplir el número de argumentos
	    // necesarios en la funcion_hebra y poder pasarla como
	    // argumento


double f( double x ) {
  return 4.0/(1+x*x);
}

double calcular_integral_secuencial(  ) {
  double suma = 0.0 ;                        // inicializar suma
  for( long i = 0 ; i < MUESTRAS ; i++ )            // para cada $i$ entre
					     // $0$ y $m-1$: 
    suma += f( (i+double(0.5)) /MUESTRAS );         //   $~$ añadir $f(x_i)$
					     //   a la suma actual 
  return suma/MUESTRAS ;                            // devolver valor
					     // promedio de $f$
}



double funcion_hebra(long ih) {
  double sum = 0;
  for(int i = ih; i < MUESTRAS; i+= hebras)
    sum += f(((double) i + 0.5)/MUESTRAS);
  return sum/MUESTRAS;
}


double calcular_integral_concurrente(){
  vector<future<double>> v;

  for(int i = 0; i < hebras; i++)
    v.push_back(async(launch::async,funcion_hebra,i));

  double total = 0;
  for(int i = 0; i < hebras; i++)
    total += v[i].get();

  return total;
  
	  
  

}




int main(int argc, char* argv[]) {
  
  if(argc != 2) {
  cout << "Error de sintáxis. Uso correcto: ./ejecutable <n_hebras_multiplo_de_2>" << endl; 
  return 1;
}
  
  hebras = atoi(argv[1]);

  time_point<steady_clock> inicio_sec  = steady_clock::now() ;
  const double             result_sec  = calcular_integral_secuencial(  );
  time_point<steady_clock> fin_sec     = steady_clock::now() ;
  double x = sin(0.4567);
  time_point<steady_clock> inicio_conc = steady_clock::now() ;
  const double             result_conc = calcular_integral_concurrente(  );
  time_point<steady_clock> fin_conc    = steady_clock::now() ;
  duration<float,milli>    tiempo_sec  = fin_sec  - inicio_sec ,
    tiempo_conc = fin_conc - inicio_conc ;
  const float              porc        = 100.0*tiempo_conc.count()/tiempo_sec.count() ;


  constexpr double pi = 3.14159265358979323846l ;

  cout << "Número de muestras (m)   : " << MUESTRAS << endl
       << "Número de hebras (n)     : " << hebras << endl
       << setprecision(18)
       << "Valor de PI              : " << pi << endl
       << "Resultado secuencial     : " << result_sec  << endl
       << "Resultado concurrente    : " << result_conc << endl
       << setprecision(5)
       << "Tiempo secuencial        : " << tiempo_sec.count()  << " milisegundos. " << endl
       << "Tiempo concurrente       : " << tiempo_conc.count() << " milisegundos. " << endl
       << setprecision(4)
       << "Porcentaje t.conc/t.sec. : " << porc << "%" << endl;

  
  
  
}
  

  // La ejecución secuencial siguiendo el ejemplo era la siguiente

  /*
    const long m = ..., n = ...; // el valor m es alto (del orden de millones)
    // implementa función f

    double f( double x ) {
    return 4.0/(1+x*x) ;
    //
    f ( x ) = 4/ ( 1 + x 2 )
    }
    // calcula la integral de forma secuencial, devuelve resultado:
    double calcular_integral_secuencial( long m ) // m == núm. muestras
    {
    double suma = 0.0 ;
    // inicializar suma
    for( long i = 0 ; i < m ; i++ )
    // para cada i entre 0 y m − 1:
    suma += f( (i+double(0.5))/m );
    // añadir f ( x i ) a la suma actual
    return suma/m ;
    // devolver valor promedio de f
    }
  */
