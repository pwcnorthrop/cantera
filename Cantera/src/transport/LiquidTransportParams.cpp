/**
 *  @file LiquidTransportParams.cpp
 *  Source code for liquid mixture transport property evaluations.
 */
/*
 *  $Author: jchewso $
 *  $Date: 2009-11-22 17:34:46 -0700 (Mon, 16 Nov 2009) $
 *  $Revision: 261 $
 *
 */

#include "LiquidTransportParams.h"
#include <iostream>
#include "IonsFromNeutralVPSSTP.h"
#include "MargulesVPSSTP.h"
#include <stdlib.h>
using namespace std;


namespace Cantera {

  /** 
   * Exception thrown if an error is encountered while reading the 
   * transport database.
   */
  class LTPError : public CanteraError {
  public:
    LTPError( std::string msg ) 
      : CanteraError("LTPspecies",
		     "error parsing transport data: " 
		     + msg + "\n") {}
  };

  /** 
   * Exception thrown if an error is encountered while reading the 
   * transport database.
   */
  class LTPmodelError : public CanteraError {
  public:
    LTPmodelError( std::string msg ) 
      : CanteraError("LTPspecies",
		     "error parsing transport data: " 
		     + msg + "\n") {}
  };


  //!Constructor
  /**
   *  @param tp_ind          Index indicating transport property type (i.e. viscosity) 
   */
  LiquidTranInteraction::LiquidTranInteraction( TransportPropertyList tp_ind ) : 
    m_model(LTI_MODEL_NOTSET),
    m_property(tp_ind)
  {
  }

  LiquidTranInteraction::~LiquidTranInteraction(){
    int kmax = m_Aij.size();
    for ( int k = 0; k < kmax; k++)
      if ( m_Aij[k] ) delete m_Aij[k];
    kmax = m_Bij.size();
    for ( int k = 0; k < kmax; k++)
      if ( m_Bij[k] ) delete m_Bij[k];
    kmax = m_Hij.size();
    for ( int k = 0; k < kmax; k++)
      if ( m_Hij[k] ) delete m_Hij[k];
    kmax = m_Sij.size();
    for ( int k = 0; k < kmax; k++)
      if ( m_Sij[k] ) delete m_Sij[k];
  }


  void LiquidTranInteraction::init( const XML_Node &compModelNode, 
			       thermo_t* thermo ) 
  {
    
    doublereal poly0;
    m_thermo = thermo;

    int nsp = thermo->nSpecies();
    m_Dij.resize( nsp, nsp, 0.0 );
    m_Eij.resize( nsp, nsp, 0.0 );
    /*
    m_Aij.resize( nsp);
    m_Bij.resize( nsp);
    m_Hij.resize( nsp);
    m_Sij.resize( nsp);
    for ( int k = 0; k < nsp; k++ ){
      (*m_Aij[k]).resize( nsp, nsp, 0.0);
      (*m_Bij[k]).resize( nsp, nsp, 0.0);
      (*m_Hij[k]).resize( nsp, nsp, 0.0);
      (*m_Sij[k]).resize( nsp, nsp, 0.0);  
    }
    */

    std::string speciesA;
    std::string speciesB;

    int num = compModelNode.nChildren();
    for (int iChild = 0; iChild < num; iChild++) {
      XML_Node &xmlChild = compModelNode.child(iChild);
      std::string nodeName = lowercase( xmlChild.name() );
      if ( nodeName != "interaction"  )	    
	throw CanteraError("TransportFactory::getLiquidInteractionsTransportData", 
			   "expected <interaction> element and got <" + nodeName + ">" );
      speciesA = xmlChild.attrib("speciesA");
      speciesB = xmlChild.attrib("speciesB");
      int iSpecies = m_thermo->speciesIndex( speciesA );
      if ( iSpecies < 0 ) 
	throw CanteraError("TransportFactory::getLiquidInteractionsTransportData", 
			   "Unknown species " + speciesA );
      int jSpecies = m_thermo->speciesIndex( speciesB );
      if ( jSpecies < 0 ) 
	throw CanteraError("TransportFactory::getLiquidInteractionsTransportData", 
			   "Unknown species " + speciesB );
      /*      if ( xmlChild.hasChild( "Aij" ) ) {
	m_Aij(iSpecies,jSpecies) = getFloat( xmlChild, "Aij", "toSI" );
	m_Aij(jSpecies,iSpecies) = m_Aij(iSpecies,jSpecies) ;
	}*/

      if ( xmlChild.hasChild( "Eij" ) ) {
	m_Eij(iSpecies,jSpecies) = getFloat( xmlChild, "Eij", "actEnergy" );
	m_Eij(iSpecies,jSpecies) /= GasConstant;
	m_Eij(jSpecies,iSpecies) = m_Eij(iSpecies,jSpecies) ;
      }

      if ( xmlChild.hasChild( "Aij" ) ) {	
        vector_fp poly;
        poly0 = getFloat( poly, xmlChild, "Aij", "toSI" );
        if ( !poly.size() ) poly.push_back(poly0);
        while (m_Aij.size()<poly.size()){
            DenseMatrix * aTemp = new DenseMatrix();
            aTemp->resize( nsp, nsp, 0.0);
            m_Aij.push_back(aTemp);
          }
	for( int i=0; i<(int)poly.size(); i++ ){
          (*m_Aij[i])(iSpecies,jSpecies) = poly[i];
	  //(*m_Aij[i])(jSpecies,iSpecies) = (*m_Aij[i])(iSpecies,jSpecies) ;
	}
      }

      if ( xmlChild.hasChild( "Bij" ) ) {	
        vector_fp poly;
        poly0 = getFloat( poly, xmlChild, "Bij", "toSI" );
        if ( !poly.size() ) poly.push_back(poly0);
        while (m_Bij.size()<poly.size()){
            DenseMatrix * bTemp = new DenseMatrix();
            bTemp->resize( nsp, nsp, 0.0);
            m_Bij.push_back(bTemp);
          }
	for( int i=0; i<(int)poly.size(); i++ ){
          (*m_Bij[i])(iSpecies,jSpecies) = poly[i];
	  //(*m_Bij[i])(jSpecies,iSpecies) = (*m_Bij[i])(iSpecies,jSpecies) ;
	}
      }

      if ( xmlChild.hasChild( "Hij" ) ) {	
        vector_fp poly;
        poly0 = getFloat( poly, xmlChild, "Hij", "actEnergy" );
        if ( !poly.size() ) poly.push_back(poly0);
        while (m_Hij.size()<poly.size()){
            DenseMatrix * hTemp = new DenseMatrix();
            hTemp->resize( nsp, nsp, 0.0);
            m_Hij.push_back(hTemp);
          }
	for( int i=0; i<(int)poly.size(); i++ ){
          (*m_Hij[i])(iSpecies,jSpecies) = poly[i];
	  (*m_Hij[i])(iSpecies,jSpecies) /= GasConstant;
	  //(*m_Hij[i])(jSpecies,iSpecies) = (*m_Hij[i])(iSpecies,jSpecies) ;
	}
      }

      if ( xmlChild.hasChild( "Sij" ) ) {	
        vector_fp poly;
        poly0 = getFloat( poly, xmlChild, "Sij", "actEnergy" );
        if ( !poly.size() ) poly.push_back(poly0);
        while (m_Sij.size()<poly.size()){
            DenseMatrix * sTemp = new DenseMatrix();
            sTemp->resize( nsp, nsp, 0.0);
            m_Sij.push_back(sTemp);
          }
	for( int i=0; i<(int)poly.size(); i++ ){
          (*m_Sij[i])(iSpecies,jSpecies) = poly[i];
	  (*m_Sij[i])(iSpecies,jSpecies) /= GasConstant;
	  //(*m_Sij[i])(jSpecies,iSpecies) = (*m_Sij[i])(iSpecies,jSpecies) ;
	}
      }

      /*0      if ( xmlChild.hasChild( "Sij" ) ) {
	m_Sij(iSpecies,jSpecies) = getFloat( xmlChild, "Sij", "toSI" );
	m_Sij(iSpecies,jSpecies) /= GasConstant;
	//m_Sij(jSpecies,iSpecies) = m_Sij(iSpecies,jSpecies) ;
	}*/

      if ( xmlChild.hasChild( "Dij" ) ) {
	m_Dij(iSpecies,jSpecies) = getFloat( xmlChild, "Dij", "toSI" );
	m_Dij(jSpecies,iSpecies) = m_Dij(iSpecies,jSpecies) ;
      }
    }
  }

  //! Copy constructor
  LiquidTranInteraction::LiquidTranInteraction( const LiquidTranInteraction &right ) {
    *this = right;  //use assignment operator to do other work
  }
   
  //! Assignment operator
  LiquidTranInteraction& LiquidTranInteraction::operator=( const LiquidTranInteraction &right )
  {
    if (&right != this) {
      m_model     = right.m_model;
      m_property  = right.m_property;
      m_thermo    = right.m_thermo;
      //m_trParam   = right.m_trParam;
      m_Aij       = right.m_Aij;
      m_Bij       = right.m_Bij;
      m_Eij       = right.m_Eij;
      m_Hij       = right.m_Hij;
      m_Sij       = right.m_Sij;
      m_Dij       = right.m_Dij;
    }
    return *this;     
  }
  
  //====================================================================================================================
  LiquidTransportParams::LiquidTransportParams() :
    viscosity(0), ionConductivity(0), thermalCond(0), speciesDiffusivity(0), electCond(0), hydroRadius(0), model_viscosity(LTI_MODEL_NOTSET),
    model_speciesDiffusivity(LTI_MODEL_NOTSET), model_hydroradius(LTI_MODEL_NOTSET)
  {
    
  }
  //====================================================================================================================
  LiquidTransportParams::~LiquidTransportParams()
  {
    delete viscosity;
    delete ionConductivity;
    delete thermalCond;
    delete speciesDiffusivity;
    delete electCond;
    delete hydroRadius;
  }

  //====================================================================================================================
  LiquidTransportParams::LiquidTransportParams(const LiquidTransportParams &right) :
   viscosity(0), thermalCond(0), speciesDiffusivity(0), electCond(0), hydroRadius(0), model_viscosity(LTI_MODEL_NOTSET),
    model_speciesDiffusivity(LTI_MODEL_NOTSET), model_hydroradius(LTI_MODEL_NOTSET)
  {
    throw CanteraError("LiquidTransportParams(const LiquidTransportParams &right)","not implemented");
  }

 //====================================================================================================================
 
  LiquidTransportParams&  LiquidTransportParams::operator=(const LiquidTransportParams & right) 
  {
   if (&right != this) {
      return *this;
    }

   throw CanteraError("LiquidTransportParams(const LiquidTransportParams &right)","not implemented");
   return *this;

  }

  //====================================================================================================================
 


  doublereal LTI_Solvent::getMixTransProp( doublereal *speciesValues, doublereal *speciesWeight ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    //if weightings are specified, use those
    if ( speciesWeight ) {  
      for ( int k = 0; k < nsp; k++) { 
	molefracs[k] = molefracs[k];
        // should be: molefracs[k] = molefracs[k]*speciesWeight[k]; for consistency, but weight(solvent)=1?
      }
    }
    else {
      throw CanteraError("LTI_Solvent::getMixTransProp","You should be specifying the speciesWeight");
	  /*  //This does not follow directly a solvent model 
      //although if the solvent mole fraction is dominant 
      //and the other species values are given or zero, 
      //it should work.
      for ( int k = 0; k < nsp; k++) { 
	value += speciesValues[k] * molefracs[k];
	}*/
    }

    for ( int i = 0; i < nsp; i++ ){
      //presume that the weighting is set to 1.0 for solvent and 0.0 for everything else.
      value += speciesValues[i] * speciesWeight[i];
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Aij[k])(i,j)*pow(molefracs[i],k);
	}
	for ( int k = 0; k < (int)m_Bij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Bij[k])(i,j)*temp*pow(molefracs[i],k);
	}      }
    }

    return value;
  }


  doublereal LTI_Solvent::getMixTransProp( std::vector<LTPspecies*> LTPptrs ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    

    for ( int k = 0; k < nsp; k++) { 
      molefracs[k] = molefracs[k];
      // should be:      molefracs[k] = molefracs[k]*LTPptrs[k]->getMixWeight(); for consistency, but weight(solvent)=1?
    }

    for ( int i = 0; i < nsp; i++ ){
      //presume that the weighting is set to 1.0 for solvent and 0.0 for everything else.
      value += LTPptrs[i]->getSpeciesTransProp() * LTPptrs[i]->getMixWeight();
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Aij[k])(i,j)*pow(molefracs[i],k);
	}
	for ( int k = 0; k < (int)m_Bij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Bij[k])(i,j)*temp*pow(molefracs[i],k);
	}
      }
    }

    return value;
  }


  


  doublereal LTI_MoleFracs::getMixTransProp( doublereal *speciesValues, doublereal *speciesWeight ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    //if weightings are specified, use those
    if ( speciesWeight ) {  
      for ( int k = 0; k < nsp; k++) { 
	molefracs[k] = molefracs[k]*speciesWeight[k];
      }
    }
    else {
      throw CanteraError("LTI_MoleFracs::getMixTransProp","You should be specifying the speciesWeight");
    }

    for ( int i = 0; i < nsp; i++ ){
      value += speciesValues[i] * molefracs[i];
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Aij[k])(i,j)*pow(molefracs[i],k);
	}
	for ( int k = 0; k < (int)m_Bij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Bij[k])(i,j)*temp*pow(molefracs[i],k);
	}
      }
    }

    return value;
  }


  doublereal LTI_MoleFracs::getMixTransProp( std::vector<LTPspecies*> LTPptrs ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    

    for ( int k = 0; k < nsp; k++) { 
      molefracs[k] = molefracs[k]*LTPptrs[k]->getMixWeight();
    }
    
    for ( int i = 0; i < nsp; i++ ){
      value += LTPptrs[i]->getSpeciesTransProp() * molefracs[i];
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Aij[k])(i,j)*pow(molefracs[i],k);
	}
	for ( int k = 0; k < (int)m_Bij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Bij[k])(i,j)*temp*pow(molefracs[i],k);
	}
      }
    }
    return value;
  }


  doublereal LTI_MassFracs::getMixTransProp( doublereal *speciesValues, doublereal *speciesWeight ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal massfracs[nsp];
    m_thermo->getMassFractions(massfracs);

    doublereal value = 0;    
    
    //if weightings are specified, use those
    if ( speciesWeight ) {  
      for ( int k = 0; k < nsp; k++) { 
	massfracs[k] = massfracs[k]*speciesWeight[k];
      }
    }
    else {
      throw CanteraError("LTI_MassFracs::getMixTransProp","You should be specifying the speciesWeight");
    }

    for ( int i = 0; i < nsp; i++ ){
      value += speciesValues[i] * massfracs[i];
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += massfracs[i]*massfracs[j]*(*m_Aij[k])(i,j)*pow(massfracs[i],k);
	}
	for ( int k = 0; k < (int)m_Bij.size(); k++ ){
          value += massfracs[i]*massfracs[j]*(*m_Bij[k])(i,j)*temp*pow(massfracs[i],k);
	}
      }
    }

    return value;
  }


  doublereal LTI_MassFracs::getMixTransProp( std::vector<LTPspecies*> LTPptrs ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal massfracs[nsp];
    m_thermo->getMassFractions(massfracs);

    doublereal value = 0;    

    for ( int k = 0; k < nsp; k++) { 
      massfracs[k] = massfracs[k]*LTPptrs[k]->getMixWeight();
    }
    
    for ( int i = 0; i < nsp; i++ ){
      value += LTPptrs[i]->getSpeciesTransProp() * massfracs[i];
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += massfracs[i]*massfracs[j]*(*m_Aij[k])(i,j)*pow(massfracs[i],k);
	}
	for ( int k = 0; k < (int)m_Bij.size(); k++ ){
          value += massfracs[i]*massfracs[j]*(*m_Bij[k])(i,j)*temp*pow(massfracs[i],k);
	}
      }
    }

    return value;
  }

 


  doublereal LTI_Log_MoleFracs::getMixTransProp( doublereal *speciesValues, doublereal *speciesWeight ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);



    doublereal value = 0;    
    
    //if weightings are specified, use those
    if ( speciesWeight ) { 
      for ( int k = 0; k < nsp; k++) { 
        molefracs[k] = molefracs[k]*speciesWeight[k];
      }
    }
    else{
      throw CanteraError("LTI_Log_MoleFracs::getMixTransProp","You probably should have a speciesWeight when you call getMixTransProp to convert ion mole fractions to molecular mole fractions");
    }

    for ( int i = 0; i < nsp; i++ ){
      value += log( speciesValues[i] ) * molefracs[i];
      for ( int j = 0; j < nsp; j++ ){
	for ( int k = 0; k < (int)m_Hij.size(); k++ ){ 
	  value += molefracs[i]*molefracs[j]*(*m_Hij[k])(i,j)/temp*pow(molefracs[i],k);
	  //cout << "value = " << value << ", m_Sij = " << (*m_Sij[k])(i,j) << ", m_Hij = " << (*m_Hij[k])(i,j) << endl;
        }
	for ( int k = 0; k < (int)m_Sij.size(); k++ ){ 
	  value -= molefracs[i]*molefracs[j]*(*m_Sij[k])(i,j)*pow(molefracs[i],k);
	  //cout << "value = " << value << ", m_Sij = " << (*m_Sij[k])(i,j) << ", m_Hij = " << (*m_Hij[k])(i,j) << endl;
        }
      }
    }

    value = exp( value );
    return value;
  }


  doublereal LTI_Log_MoleFracs::getMixTransProp(std::vector<LTPspecies*> LTPptrs) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);
    

    doublereal value = 0;    

    //if weightings are specified, use those

    for ( int k = 0; k < nsp; k++) { 
      molefracs[k] = molefracs[k]*LTPptrs[k]->getMixWeight( );
    }

    for ( int i = 0; i < nsp; i++ ){
      value += log( LTPptrs[i]->getSpeciesTransProp() ) * molefracs[i];
      for ( int j = 0; j < nsp; j++ ){
	for ( int k = 0; k < (int)m_Hij.size(); k++ ){ 
	  value +=  molefracs[i]*molefracs[j]*(*m_Hij[k])(i,j)/temp*pow(molefracs[i],k);
          //cout << "1 = " << molefracs[i]+molefracs[j] << endl;
	  //cout << "value = " << value << ", m_Sij = " << (*m_Sij[k])(i,j) << ", m_Hij = " << (*m_Hij[k])(i,j) << endl;
        }
	for ( int k = 0; k < (int)m_Sij.size(); k++ ){ 
	  value -=  molefracs[i]*molefracs[j]*(*m_Sij[k])(i,j)*pow(molefracs[i],k);
          //cout << "1 = " << molefracs[i]+molefracs[j] << endl;
	  //cout << "value = " << value << ", m_Sij = " << (*m_Sij[k])(i,j) << ", m_Hij = " << (*m_Hij[k])(i,j) << endl;
        }
      }
    }

    value = exp( value );
    //    cout << ", viscSpeciesA = " << LTPptrs[0]->getSpeciesTransProp() << endl;
    //cout << ", viscSpeciesB = " << LTPptrs[1]->getSpeciesTransProp() << endl;
    //cout << "value = " << value << " FINAL" << endl;
    return value;
  }


  


  void LTI_Pairwise_Interaction::setParameters(LiquidTransportParams& trParam) {
    int nsp = m_thermo->nSpecies();
    m_diagonals.resize(nsp, 0);

    for (int k = 0; k < nsp; k++) {
      Cantera::LiquidTransportData &ltd = trParam.LTData[k];
      if (ltd.speciesDiffusivity)
	m_diagonals[k] = ltd.speciesDiffusivity;
    }
  }

  doublereal LTI_Pairwise_Interaction::getMixTransProp(doublereal *speciesValues, doublereal *speciesWeight) {

    int nsp = m_thermo->nSpecies();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    throw LTPmodelError( "Calling LTI_Pairwise_Interaction::getMixTransProp does not make sense." );

    return value;
  }


  doublereal LTI_Pairwise_Interaction::getMixTransProp(std::vector<LTPspecies*> LTPptrs) {

    int nsp = m_thermo->nSpecies();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    throw LTPmodelError( "Calling LTI_Pairwise_Interaction::getMixTransProp does not make sense." );

    return value;
  }

  void LTI_Pairwise_Interaction::getMatrixTransProp( DenseMatrix &mat, doublereal *speciesValues ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions( molefracs );

    mat.resize( nsp, nsp, 0.0 );
    for ( int i = 0; i < nsp; i++ ) 
      for ( int j = 0; j < i; j++ ) 
	mat(i,j) = mat(j,i) = exp( m_Eij(i,j) / temp ) / m_Dij(i,j);
    
    for ( int i = 0; i < nsp; i++ ) 
      if ( mat(i,i) == 0.0 && m_diagonals[i] ) 
	mat(i,i) = 1.0 / m_diagonals[i]->getSpeciesTransProp() ;
  }


  void LTI_StefanMaxwell_PPN::setParameters( LiquidTransportParams& trParam ) {
    int nsp = m_thermo->nSpecies();
    int nBinInt = nsp*(nsp-1)/2;
    //vector<std
    
    m_ionCondMix = 0;
    m_ionCondMixModel = trParam.ionConductivity;
    //trParam.ionConductivity = 0;
    m_ionCondSpecies.resize(nsp,0);
    m_mobRatMix.resize(nsp,nsp,0.0);
    m_mobRatMixModel.resize(nBinInt);
    m_mobRatSpecies.resize(nBinInt);
    m_mobRatIndex.resize(nBinInt);
    m_selfDiffMix.resize(nsp,0.0);
    m_selfDiffMixModel.resize(nsp);
    m_selfDiffSpecies.resize(nsp);
    m_selfDiffIndex.resize(nsp);

    for ( int k = 0; k < nBinInt; k++ ) {
      m_mobRatMixModel[k] = trParam.mobilityRatio[k];
      //trParam.mobilityRatio[k] = 0;
      m_mobRatSpecies[k].resize(nsp,0);
      m_mobRatIndex[k] = trParam.mobRatIndex[k];
    }
    for ( int k = 0; k < nsp; k++ ) {
      m_selfDiffMixModel[k] = trParam.selfDiffusion[k];
      //trParam.selfDiffusion[k] = 0;
      m_selfDiffSpecies[k].resize(nsp,0);
      m_selfDiffIndex[k] = trParam.selfDiffIndex[k];
    }

    for (int k = 0; k < nsp; k++) {
      Cantera::LiquidTransportData &ltd = trParam.LTData[k];
      m_ionCondSpecies[k]   =  ltd.ionConductivity;
      //ltd.ionConductivity = 0;
      for ( int j = 0; j < nBinInt; j++ ){
	m_mobRatSpecies[j][k] = ltd.mobilityRatio[j];
	//ltd.mobilityRatio[j] = 0;
      }
      for ( int j = 0; j < nsp; j++ ){
	m_selfDiffSpecies[j][k] = ltd.selfDiffusion[j]; 
	//ltd.selfDiffusion[j] = 0;      
      }
    }
  }

  doublereal LTI_StefanMaxwell_PPN::getMixTransProp( doublereal *speciesValues, doublereal *speciesWeight ) {

    int nsp = m_thermo->nSpecies();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    throw LTPmodelError( "Calling LTI_StefanMaxwell_PPN::getMixTransProp does not make sense." );

    return value;
  }


  doublereal LTI_StefanMaxwell_PPN::getMixTransProp( std::vector<LTPspecies*> LTPptrs ) {

    int nsp = m_thermo->nSpecies();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    throw LTPmodelError( "Calling LTI_StefanMaxwell_PPN::getMixTransProp does not make sense." );

    return value;
  }
  
  void LTI_StefanMaxwell_PPN::getMatrixTransProp( DenseMatrix &mat, doublereal *speciesValues ) {
    //CAL
    
    IonsFromNeutralVPSSTP * ions_thermo = dynamic_cast<IonsFromNeutralVPSSTP *>(m_thermo);
    int i, j, k;
    int nsp = m_thermo->nSpecies();
    if (nsp != 3) throw CanteraError("LTI_StefanMaxwell_PPN::getMatrixTransProp","Function may only be called with a 3-ion system");
    int nBinInt = nsp*(nsp-1)/2;
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions( molefracs );
    vector_fp neut_molefracs;
    ions_thermo->getNeutralMolecMoleFractions(neut_molefracs);
    vector<int> cation;
    vector<int> anion;
    ions_thermo->getCationList(cation);
    ions_thermo->getAnionList(anion);
    vector<std::string> speciesNames;
    ions_thermo->getSpeciesNames(speciesNames);

    // Reaction Coeffs and Charges
    std::vector<double> viS(6);
    std::vector<double> charges(3);
    ions_thermo->getDissociationCoeffs(viS,charges);

    if ((int)anion.size() != 1) 
      throw CanteraError("LTI_StefanMaxwell_PPN::getMatrixTransProp","Must have one anion only for StefanMaxwell_PPN");
    if ((int)cation.size() != 2) 
      throw CanteraError("LTI_StefanMaxwell_PPN::getMatrixTransProp","Must have two cations of equal charge for StefanMaxwell_PPN");
    if (charges[cation[0]] != charges[cation[1]]) 
      throw CanteraError("LTI_StefanMaxwell_PPN::getMatrixTransProp","Cations must be of equal charge for StefanMaxwell_PPN");

    /*
    cout << "cation 0: " << speciesNames[cation[0]] << endl;
    cout << "cation 1: " << speciesNames[cation[1]] << endl;
    cout << "anion 0: " << speciesNames[anion[0]] << endl;
    */

    m_ionCondMix = m_ionCondMixModel->getMixTransProp(m_ionCondSpecies);

    MargulesVPSSTP * marg_thermo = dynamic_cast<MargulesVPSSTP *> (ions_thermo->neutralMoleculePhase_);
    doublereal vol = m_thermo->molarVolume();

    typedef std::vector<int> intVec;
    std::vector<intVec> mobRatIndexMap;
    mobRatIndexMap.resize(nsp);
    for ( k = 0; k < nsp; k++ )
      mobRatIndexMap[k].resize(nsp,0);

    for ( k = 0; k < nBinInt; k++ ) {
      bool missingIndex = 1;
      for ( i = 0; i < nsp; i++ ) {
	for ( j = 0; j < i; j++ ) {
	  if ( (speciesNames[i] + ":" + speciesNames[j]) == m_mobRatIndex[k] ) {
	    mobRatIndexMap[i][j] = k;
	    m_mobRatMix(i,j) = m_mobRatMixModel[k]->getMixTransProp( m_mobRatSpecies[k] );	
	    if ( m_mobRatMix(i,j) > 0 ) m_mobRatMix(j,i) = 1.0/m_mobRatMix(i,j);
	    missingIndex = 0;
	    break;
	  }
	  else if ( (speciesNames[j] + ":" + speciesNames[i]) == m_mobRatIndex[k] ) {
	    m_mobRatMix(j,i) = m_mobRatMixModel[k]->getMixTransProp( m_mobRatSpecies[k] );
	    if ( m_mobRatMix(j,i) > 0 ) m_mobRatMix(i,j) = 1.0/m_mobRatMix(j,i);
	    missingIndex = 0;
	    break;
	  }
	}
      }
      if ( missingIndex ) throw CanteraError("LTI_StefanMaxwell_PPN::getMixTransProp","Incorrect names for mobility ratio of " + m_mobRatIndex[k] + " rather than i.e. " + speciesNames[0] + ":" + speciesNames[1]); 
    } 

    
    for ( k = 0; k < nsp; k++ ){
      j = 0;
      while (m_selfDiffIndex[k] != speciesNames[j]) {
	j++;
	if (j == nsp) throw CanteraError("LTI_StefanMaxwell_PPN::getMixTransProp","Incorrect names for self diffusion of " + m_selfDiffIndex[k] + " rather than i.e. " + speciesNames[0]); 
      } 
      m_selfDiffMix[j] = m_selfDiffMixModel[k]->getMixTransProp( m_selfDiffSpecies[k] );
    }

    /*
    for ( i = 0; i < nsp; i++ ) {
      cout << "D" << i << "* = " << m_selfDiffMix[i] << endl;
      for ( j = 0; j < nsp; j++ ) {
        cout << "ratio" << i << j << " = " << m_mobRatMix(i,j) << endl;
      }
    }
    */
    
    int vP = max(viS[cation[0]],viS[cation[1]]);
    int vM = viS[anion[0]];
    int zP = charges[cation[0]];
    int zM = charges[anion[0]];
    doublereal xA, xB, eps;    
    doublereal inv_vP_vM_MutualDiff;
    vector_fp dlnActCoeffdlnN;
    dlnActCoeffdlnN.resize(neut_molefracs.size(),0.0);

    std::string cationIndex (4,'0');
    for ( i = 0; i < 2; i++ )
      for ( j = 0; j < 2; j++ )
	if ( viS[i*nsp+cation[j]] > 0 )
	  cationIndex[i*2+j] = '1';

    if ( (cationIndex == "1001") | (cationIndex == "0110") ) {
      xA = neut_molefracs[cation[0]];
      xB = neut_molefracs[cation[1]];
      eps = (1-m_mobRatMix(cation[1],cation[0]))/(xA+xB*m_mobRatMix(cation[1],cation[0]));
      marg_thermo->getdlnActCoeffdlnN(&dlnActCoeffdlnN[0]);
      inv_vP_vM_MutualDiff = (xA*(1+dlnActCoeffdlnN[cation[1]])/m_selfDiffMix[cation[1]]+xB*(1+dlnActCoeffdlnN[cation[0]])/m_selfDiffMix[cation[0]]);
      //marg_thermo->getdlnActCoeffdlnX(&dlnActCoeffdlnN[0]);
      //inv_vP_vM_MutualDiff = (xA*(1+dlnActCoeffdlnN[cation[1]])/m_selfDiffMix[cation[1]]+xB*(1+dlnActCoeffdlnN[cation[0]])/m_selfDiffMix[cation[0]]);
    }
    else
      throw CanteraError("LTI_StefanMaxwell_PPN::getMixTransProp","Dissociation reactions don't make sense: cationIndex = " + cationIndex);
    
    mat.resize( nsp, nsp, 0.0 );
    mat(cation[0],cation[1]) = mat(cation[1],cation[0]) = (1+vM/vP)*(1+eps*xB)*(1-eps*xA)*inv_vP_vM_MutualDiff-zP*zP*Faraday*Faraday/GasConstant/temp/m_ionCondMix/vol;
  mat(cation[0],anion[0]) = mat(anion[0],cation[0]) = (1+vP/vM)*(-eps*xB*(1-eps*xA)*inv_vP_vM_MutualDiff)-zP*zM*Faraday*Faraday/GasConstant/temp/m_ionCondMix/vol;
mat(cation[1],anion[0]) = mat(anion[0],cation[1]) = (1+vP/vM)*(eps*xA*(1+eps*xB)*inv_vP_vM_MutualDiff)-zP*zM*Faraday*Faraday/GasConstant/temp/m_ionCondMix/vol;

/*
    for ( i = 0; i < nsp; i++ ) {
      for ( j = 0; j < nsp; j++ ) {
	mat(i,j) = 1.0/mat(i,j);
	//cout << "D" << i << j << " = " << mat(i,j) << endl;
      }
    }
*/
  }
  
  
  doublereal LTI_StokesEinstein::getMixTransProp( doublereal *speciesValues, doublereal *speciesWeight ) {

    int nsp = m_thermo->nSpecies();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    throw LTPmodelError( "Calling LTI_StokesEinstein::getMixTransProp does not make sense." );

    return value;
  }


  doublereal LTI_StokesEinstein::getMixTransProp( std::vector<LTPspecies*> LTPptrs ) {

    int nsp = m_thermo->nSpecies();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    throw LTPmodelError( "Calling LTI_StokesEinstein::getMixTransProp does not make sense." );

    return value;
  }



  void LTI_StokesEinstein::setParameters( LiquidTransportParams& trParam ) {
    int nsp = m_thermo->nSpecies();
    m_viscosity.resize( nsp, 0 );
    m_hydroRadius.resize( nsp, 0 );
    for (int k = 0; k < nsp; k++) {
      Cantera::LiquidTransportData &ltd = trParam.LTData[k];
      m_viscosity[k]   =  ltd.viscosity;
      m_hydroRadius[k] =  ltd.hydroRadius;
    }
  }

  void LTI_StokesEinstein::getMatrixTransProp( DenseMatrix &mat, doublereal *speciesValues ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();

    double *viscSpec = new double(nsp);
    double *radiusSpec = new double(nsp);

    for (int k = 0; k < nsp; k++) {
      viscSpec[k] = m_viscosity[k]->getSpeciesTransProp() ;
      radiusSpec[k] = m_hydroRadius[k]->getSpeciesTransProp() ;
    } 

    mat.resize(nsp,nsp, 0.0);
    for (int i = 0; i < nsp; i++) 
      for (int j = 0; j < nsp; j++) {
	mat(i,j) = ( 6.0 * Pi * radiusSpec[i] * viscSpec[j] ) / GasConstant / temp;
      }
    delete radiusSpec;
    delete viscSpec;
  }

  doublereal LTI_MoleFracs_ExpT::getMixTransProp( doublereal *speciesValues, doublereal *speciesWeight ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    
    
    //if weightings are specified, use those
    if ( speciesWeight ) {  
      for ( int k = 0; k < nsp; k++) { 
	molefracs[k] = molefracs[k]*speciesWeight[k];
      }
    }
    else {
      throw CanteraError("LTI_MoleFracs_ExpT::getMixTransProp","You should be specifying the speciesWeight");
    }

    for ( int i = 0; i < nsp; i++ ){
      value += speciesValues[i] * molefracs[i];
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Aij[k])(i,j)*pow(molefracs[i],k)*exp((*m_Bij[k])(i,j)*temp);
	}
      }
    }

    return value;
  }


  doublereal LTI_MoleFracs_ExpT::getMixTransProp( std::vector<LTPspecies*> LTPptrs ) {

    int nsp = m_thermo->nSpecies();
    doublereal temp = m_thermo->temperature();
    doublereal molefracs[nsp];
    m_thermo->getMoleFractions(molefracs);

    doublereal value = 0;    

    for ( int k = 0; k < nsp; k++) { 
      molefracs[k] = molefracs[k]*LTPptrs[k]->getMixWeight();
    }
    
    for ( int i = 0; i < nsp; i++ ){
      value += LTPptrs[i]->getSpeciesTransProp() * molefracs[i];
      for ( int j = 0; j < nsp; j++ ){ 
	for ( int k = 0; k < (int)m_Aij.size(); k++ ){
          value += molefracs[i]*molefracs[j]*(*m_Aij[k])(i,j)*pow(molefracs[i],k)*exp((*m_Bij[k])(i,j)*temp);
	}
      }
    }
    return value;
  }


  
} //namespace Cantera