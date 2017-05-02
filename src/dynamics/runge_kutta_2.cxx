#include <dynamics/runge_kutta_2.hxx>

#include <utils/config.hxx>

namespace fs0 { namespace dynamics { namespace integrators {

    RungeKutta2::RungeKutta2() {
        tmp = nullptr;
        integration_factor = Config::instance().getIntegrationFactor();
    }

    void
    RungeKutta2::operator()( const State& s, const std::vector<DifferentialEquation>& f_expr, State& next, double H) const {

        const double base_duration = (H / integration_factor);
        std::vector<Atom>   f_un; // f(u_n)
        std::vector<Atom>   un;
        un.resize( f_expr.size() );

        while ( H > 0.0 ) {
            double h = std::min( base_duration, H  );

            evaluate_derivatives( next, f_expr, f_un );
            for ( unsigned i = 0; i < f_expr.size(); i++ ) {
                //! Save values at step n
                un[i] = Atom( f_expr[i]._affected, next.getValue(f_expr[i]._affected));
                //! Step towards halfway
                //! u_{n+1/2} = u_{n} + 0.5 h f(u_{n})
                float f_i = boost::get<float>(next.getValue( f_expr[i]._affected ));
                float un_i = boost::get<float>(f_un[i].getValue());
                float un1 = f_i + 0.5f * h * un_i;
                next.set( f_expr[i]._affected, un1 );
            }
            //! evaluate the derivatives at the half-way point
            evaluate_derivatives( next, f_expr, f_un );
            for ( unsigned i = 0; i < f_expr.size(); i++ ) {
                //! Step towards next
                //! u_{n+1} = u_{n} +  h f(u_{n+1/2})
                float un_i = boost::get<float>(un[i].getValue());
                float f_un_i = boost::get<float>(f_un[i].getValue());
                float un1 = un_i +  h * f_un_i;
                next.set( f_expr[i]._affected, un1 );
            }

            H -= h;
        }

    }

}}}
