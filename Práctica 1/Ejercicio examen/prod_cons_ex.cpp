#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 40 ,   // número de items
	       tam_vec   = 10 ;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación:
				      // producidos 
          cont_cons[num_items] = {0}; // contadores de verificación:
				      // consumidos

int vect[tam_vec];                    // Vector buffer
Semaphore ocupadas = Semaphore(0);    // Semáforo de datos producidos
Semaphore libres = Semaphore(tam_vec);// Semáforo de plazas disponibles

// Apartado 1 examen
Semaphore sem_turno1 = Semaphore(1);
Semaphore sem_turno2 = Semaphore(0);

// Apartado 3 examen
Semaphore consumidora_estandar = Semaphore(1);
Semaphore consumidora_multiplo = Semaphore(0);

int primera_ocupada = 0;
int primera_libre = 0;

const bool DEBUG_MODE = true; // Cambiar a 1 para mostrar mensajes de
			   // depuración 

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{

   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "producido: " << contador << endl << flush ;

   cont_prod[contador] ++ ;
   

   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora( int id ) // id será un número aleatorio entre 0 o 1
{

   for( unsigned i = id ; i < num_items ; i+=2 )
   {

// La hebra que vaya a empezar escribiendo en la posición 0 escribirá primero
	if(id==0){
		sem_turno1.sem_wait();
	}
	else{
		sem_turno2.sem_wait();
	}
	
      libres.sem_wait();
      vect[primera_libre] = producir_dato() ;

      if(DEBUG_MODE)
	cout << "Soy hebra " << id <<" Introduciré " << vect[primera_libre] << " en la pos: " << primera_libre <<  endl;
      primera_libre++;
      primera_libre = (primera_libre % tam_vec);
      ocupadas.sem_signal();

// Posteriormente la hebra liberará el semáforo de la otra para que pueda escribir
	if(id==0){
		sem_turno2.sem_signal();
	}
	else{
		sem_turno1.sem_signal();
	}
	
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora( bool multiplo )
{
	
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
	if (multiplo){
		consumidora_multiplo.sem_wait();
		cout << "Multiplo consumido " << vect[primera_ocupada]  << " multiplo de 5" <<  endl;
		i+=4;
		consumidora_estandar.sem_signal();
	}
	else{
     		ocupadas.sem_wait();
     		consumir_dato( vect[primera_ocupada] ) ;

     		if(DEBUG_MODE)
      			 cout << "He cogido " << vect[primera_ocupada] << " de la pos: " << primera_ocupada <<  endl;
		if(i%5 == 0){
			consumidora_multiplo.sem_signal(); //Liberamos la que nos indica que era múltiplo
			consumidora_estandar.sem_wait(); // y pausamos la estándar
		}
		primera_ocupada++;
     		primera_ocupada = (primera_ocupada % tam_vec);
		
     		libres.sem_signal();
	}
    }
}


//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

// Apartado 2, decidimos de manera aleatoria quien empezará
int id = aleatorio<0,1>();
int id2;
if(id==1)
	id2=0;
else
	id2= 1;

   thread hebra_productora1 ( funcion_hebra_productora,id ),
          hebra_consumidora( funcion_hebra_consumidora, false ),
	  hebra_productora2( funcion_hebra_productora,id2 ),
          imprimir( funcion_hebra_consumidora, true );

   hebra_productora1.join() ;
   hebra_productora2.join() ;
   hebra_consumidora.join() ;
   imprimir.join();

   cout << "fin (las hebras se han unido)" << endl;

   test_contadores();
}
