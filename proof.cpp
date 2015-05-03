/*
    euler.cpp

    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#include "proof.h"
#include "parsers.h"
#include "rdf.h"

predicate *temp_preds = new predicate[max_predicates];
uint ntemppreds = 0;

reasoner::reasoner() : GND ( &predicates[npredicates++].init ( dict.set ( "GND" ) ) ) {}

predicate *predicates = new predicate[max_predicates];
rule *rules = new rule[max_rules];
proof *proofs = new proof[max_proofs];
uint npredicates = 0, nrules = 0, nproofs = 0;

reasoner::~reasoner() {
	delete[] predicates;
	delete[] rules;
	delete[] proofs;
}

rule& rule::init ( const predicate* h, predlist b ) {
	head = h;
	body = b;
	return *this;
}

proof& proof::init ( const proof& f ) {
	return init ( f.rul, f.ind, f.parent, f.substitution, f.ground );
}

proof& proof::init ( const rule* _r, uint _ind, const proof* p, subst _s, ground_t _g ) {
	if ( nproofs >= max_proofs ) throw "Buffer overflow";
	proof& f = proofs[nproofs++];
	f.rul = _r;
	f.ind = _ind;
	f.parent = p;
	f.substitution = _s;
	f.ground = _g;
	return f;
}

void reasoner::printkb() {
	static bool pause = false;
	cout << endl << "dumping kb with " << npredicates << " predicates, " << nrules << " rules and " << nproofs << " proofs. " << endl;
	cout << "predicates: " <<  endl;
	for ( uint n = 0; n < npredicates; ++n ) cout << predicates[n] << endl;
	cout << "rules: " << endl;
	for ( uint n = 0; n < nrules; ++n ) cout << rules[n] << endl;
	cout << "proofs: " << endl;
	for ( uint n = 0; n < nproofs; ++n ) cout << proofs[n] << endl;
	if ( pause ) cout << "type <enter> to continue or <c><enter> to stop pausing...";
	cout << endl;
	if ( pause && getchar() == 'c' ) pause = false;
}

const predicate* predicate::evaluate ( const map<int, const predicate*>& sub ) const {
	trace ( "\tEval " << t << " in " << sub << endl );
	if ( pred < 0 ) {
		auto it = sub.find ( pred );
		return it == sub.end() ? 0 : it->second->evaluate ( sub );
	} else { // no braces here in euler.js
		if ( args.empty() ) return this;
	}
	const predicate *p;
	predicate* r = &temp_preds[ntemppreds++].init ( pred );
	for ( auto x : args )
		r->args.emplace_back ( ( p = x->evaluate ( sub ) ) ? p : &temp_preds[ntemppreds++].init ( x->pred ) );
	return r;
}

const predicate* unify ( const predicate& s, const subst& ssub, const predicate& d, subst& dsub, bool f ) {
	trace ( "\tUnify s: " << s << " in " << ( ssub ) << " with " << d << " in " << dsub << endl );
	const predicate* p;
	if ( s.pred < 0 ) {
		if ( ( p = s.evaluate ( ssub ) ) ) return unify ( *p, ssub, d, dsub, f );
		else {
			trace ( "Match." << endl );
			return &s;
		}
	}
	if ( d.pred >= 0 ) {
		if ( s.pred != d.pred || s.args.size() != d.args.size() ) return 0;
		const predlist& as = s.args, ad = d.args;
		for ( auto sit = as.begin(), dit = ad.begin(); sit != as.end(); ++sit, ++dit )
			if ( !unify ( **sit, ssub, **dit, dsub, f ) )
				return 0;
		trace ( "Match." << endl );
		return &s;
	}
	if ( ( p = d.evaluate ( dsub ) ) ) return unify ( s, ssub, *p, dsub, f );
	if ( f ) dsub[d.pred] = s.evaluate ( ssub );
	trace ( "Match with subst: " << dsub << endl );
	return &d;
}

proof& proof::find ( const rule* goal, const cases_t& cases) {
	proof& f = init( goal );
	evidence_t e;
	f.process( cases, e );
	return f;
}

void proof::process ( const cases_t& cases, evidence_t& evidence ) {
	trace ( *this << endl );
	if ( ind >= rul->body.size() ) {
		if ( !parent )
			for ( const predicate* x : rul->body ) {
				const predicate* t = x->evaluate ( substitution );
				evidence[t->pred].emplace_back ( t, ground );
			}
		else {
			proof& new_proof = init ( parent->rul, parent->ind + 1, parent->parent, parent->substitution, ground );
			if ( !rul->body.empty() ) new_proof.ground.emplace_front ( rul, substitution );
			if ( rul->head ) unify ( *rul->head, substitution, *new_proof.rul->body[new_proof.ind - 1], new_proof.substitution, true );
			next.emplace_back( &new_proof );
		}
	} else {
		const predicate* t = rul->body[ind];
		int b = builtin ( t );
		if ( b == 1 ) {
			proof& r = proof::init ( *this );
			r.ground = ground;
			r.ground.emplace_back ( &rules[nrules++].init ( t->evaluate ( substitution ) ), subst() );
			r.ind++;
			next.push_back ( &r );
		} else if ( !b ) return;
		auto it = cases.find ( t->pred );
		if ( it != cases.end() )
			for ( const rule* rl : it->second ) {
				subst s;
				if ( !rl->head || !unify ( *t, substitution, *rl->head, s, true ) ) continue;
				const proof* ep = parent;
				for (; ep; ep = ep->parent)
					if ( ( ep->rul == rul ) && unify ( *ep->rul->head, ep->substitution, *rul->head, const_cast<subst&>(substitution), false ) )
						break;
				if ( ep && ep->parent ) continue;
				next.push_front( &proof::init ( rl, 0, this, s, rl->body.empty() ? ground_t{ { rl, subst() } } : ground_t{} ) );
			}
		}
	if (next.empty()) cout << "evidence: " << evidence << endl;
	else for (proof* f : next) f->process(cases, evidence);
}

predicate* reasoner::mkpred ( string s, const vector<const predicate*>& v ) {
	return &predicates[npredicates++].init ( dict.has ( s ) ? dict[s] : dict.set ( s ), v );
}

rule* reasoner::mkrule ( const predicate* p, const vector<const predicate*>& v ) {
	return &rules[nrules++].init ( p, v );
}

const predicate* reasoner::triple ( const string& s, const string& p, const string& o ) {
	return mkpred ( p, { mkpred ( s ), mkpred ( o ) } );
}

const predicate* reasoner::triple ( const jsonld::quad& q ) {
	return triple ( q.subj->value, q.pred->value, q.object->value );
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

evidence_t reasoner::prove ( const qdb &kb, const qlist& query ) {
	evidence_t evidence;
	cases_t cases;
	trace ( "Reasoner called with quads kb: " << endl << kb << endl << "And query: " << endl << query << endl );
	for ( const pair<string, jsonld::pqlist>& x : kb ) {
		for ( jsonld::pquad quad : *x.second ) {
			const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
			trace ( "processing quad " << quad->tostring() << endl );
			cases[dict[p]].push_back ( mkrule ( triple ( s, p, o ) ) );
			if ( p != implication || kb.find ( o ) == kb.end() ) continue;
			for ( jsonld::pquad y : *kb.at ( o ) ) {
				rule& rul = *mkrule();
				rul.head = triple ( *y );
				if ( kb.find ( s ) != kb.end() )
					for ( jsonld::pquad z : *kb.at ( s ) )
						rul.body.push_back ( triple ( *z ) );
				cases[rul.head->pred].push_back ( &rul );
				trace ( "added rule " << rul << endl );
			}
		}
	}
	rule& goal = *mkrule();
	for ( auto q : query ) goal.body.push_back ( triple ( *q ) );
//	printkb();
	return prove ( &goal, -1, cases );
}

bool reasoner::test_reasoner() {
	dict.set ( "a" );
	//	cout <<"dict:"<<endl<< dict.tostr() << endl;
	//	exit(0);
	evidence_t evidence;
	cases_t cases;
	typedef predicate* ppredicate;
	ppredicate Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {Socrates, Male} ) ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Mortal} ), { mkpred ( "a", {_x, Man } )  } ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Man   } ), { mkpred ( "a", {_x, Male} )  } ) );

	predicate* goal = mkpred ( "a", { _y, Mortal } );
	evidence = prove ( mkrule ( 0, { goal } ), -1, cases );
//	cout << "evidence: " << evidence.size() << " items..." << endl;
//	cout << evidence << endl;
//	cout << "QED!" << endl;
//	cout << evidence.size() << endl;
	return evidence.size();
}

float degrees ( float f ) {
	static float pi = acos ( -1 );
	return f * 180 / pi;
}


int proof::builtin ( const predicate* t_ ) {
	if (t_ && dict[t_->pred] == "GND") return 1;
/*	
	if ( !t_ ) return -1;
	const predicate& t = *t_;
	string p = dict[t.pred];
	if ( p == "GND" ) return 1;
	if ( t.args.size() != 2 ) throw runtime_error ( "builtin expects to get a predicate with spo" );
	const predicate* t0 = evaluate ( **t.args.begin(), f.substitution );
	const predicate* t1 = evaluate ( **++t.args.begin(), f.substitution );
	if ( p == "log:equalTo" ) return ( ( t0 && t1 && t0->pred == t1->pred ) ? 1 : 0 );
	else if ( p == "log:notEqualTo" ) return ( t0 && t1 && t0->pred != t1->pred ) ? 1 : 0;
	else if ( p == "math:absoluteValue" ) {
		if ( t0 && t0->pred > 0 )
			return ( unify ( mkpred ( tostr ( fabs ( stof ( dict[t0->pred] ) ) ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	}
#define FLOAT_BINARY_BUILTIN(x, y) \
  else if (p == x) \
      return \
        (t0 && t0->pred > 0 && \
      	(unify(mkpred(tostr(y(stof(dict[t0->pred])))), f.substitution, t.args[1], f.substitution, true))) || \
        (t1 && t1->pred > 0 && \
      	(unify(mkpred(tostr(y(stof(dict[t0->pred])))), f.substitution, t.args[0], f.substitution, true))) \
	? 1 : 0
	FLOAT_BINARY_BUILTIN ( "math:degrees", degrees );
	FLOAT_BINARY_BUILTIN ( "math:cos", cos );
	FLOAT_BINARY_BUILTIN ( "math:sin", cos );
	FLOAT_BINARY_BUILTIN ( "math:tan", cos );
	else if ( p == "math:equalTo" ) return ( t0 && t1 && stof ( dict[t0->pred] ) == stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:greaterThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) > stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:lessThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) < stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:notEqualTo" ) return ( t0 && t1 && stof ( dict[t0->pred] ) != stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:notLessThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) >= stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:notGreaterThan" ) return ( t0 && t1 && stof ( dict[t0->pred] ) <= stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:equalTo" ) return ( t0 && t1 && stof ( dict[t0->pred] ) == stof ( dict[t1->pred] ) ) ? 1 : 0;
	else if ( p == "math:difference" && t0 ) {
		float a = stof ( dict[ ( evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred )] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a -= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		return ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	} else if ( p == "math:exponentiation" && t0 ) {
		float a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; ti && !ti->args.empty(); ti = ti->args[1] ) a = pow ( a, stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] ) );
		return ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	} else if ( p == "math:integerQuotient" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a /= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		return ( unify ( mkpred ( tostr ( floor ( a ) ) ), f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	} else if ( p == "math:negation" ) {
		if ( t0 && t0->pred >= 0 ) {
			auto a = -stof ( dict[t0->pred] );
			if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		} else if ( t1 && t1->pred >= 0 ) {
			auto a = -stof ( dict[t1->pred] );
			if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[0], f.substitution, true ) ) return 1;
		} else return 0;
	} else if ( p == "math:product" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a *= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "math:quotient" && t0 ) {
		auto a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a /= stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "math:remainder" && t0 ) {
		auto a = stoi ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a %= stoi ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "math:rounded" ) {
		if ( t0 && t0->pred >= 0 ) {
			float a = round ( stof ( dict[t0->pred] ) );
			if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		} else return 0;
	} else if ( p == "math:sum" && t0 ) {
		float a = stof ( dict[evaluate ( *const_cast<const predicate*> ( t0->args[0] ), f.substitution )->pred] );
		for ( auto ti = t0->args[1]; !ti->args.empty(); ti = ti->args[1] ) a += stof ( dict[evaluate ( *const_cast<const predicate*> ( ti->args[0] ), f.substitution )->pred] );
		if ( unify ( mkpred ( tostr ( a ) ), f.substitution, t.args[1], f.substitution, true ) ) return 1;
		else return 0;
	} else if ( p == "rdf:first" && t0 && dict[t0->pred] == "." && !t0->args.empty() )
		return ( unify ( t0->args[0], f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	else if ( p == "rdf:rest" && t0 && dict[t0->pred] == "." && !t0->args.empty() )
		return ( unify ( t0->args[1], f.substitution, t.args[1], f.substitution, true ) ) ? 1 : 0;
	else if ( p == "a" && t1 && dict[t1->pred] == "rdf:List" && t0 && dict[t0->pred] == "." ) return 1;
	else if ( p == "a" && t1 && dict[t1->pred] == "rdfs:Resource" ) return 1;*/
	return -1;
}