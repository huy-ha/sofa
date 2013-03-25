cmake_minimum_required(VERSION 2.8)

project("SofaBaseLinearSolver")

include(${SOFA_CMAKE_DIR}/pre.cmake)

set(HEADER_FILES

    initBaseLinearSolver.h 
    linearsolver/CGLinearSolver.h 
    linearsolver/CGLinearSolver.inl 
    linearsolver/CholeskySolver.h 
    linearsolver/CholeskySolver.inl 
    linearsolver/BTDLinearSolver.h 
    linearsolver/BTDLinearSolver.inl 
    linearsolver/FullVector.h 
    linearsolver/FullMatrix.h 
    linearsolver/DiagonalMatrix.h 
    linearsolver/SingleMatrixAccessor.h 
    linearsolver/SparseMatrix.h 
    linearsolver/CompressedRowSparseMatrix.h 
    linearsolver/CompressedRowSparseMatrix.inl 
    linearsolver/GraphScatteredTypes.h 
    linearsolver/DefaultMultiMatrixAccessor.h 
    linearsolver/MatrixLinearSolver.h 
    linearsolver/MatrixExpr.h 
    linearsolver/matrix_bloc_traits.h 
    linearsolver/BlocMatrixWriter.h
    
    )
    
set(SOURCE_FILES

    initBaseLinearSolver.cpp 
    linearsolver/CGLinearSolver.cpp 
    linearsolver/CholeskySolver.cpp 
    linearsolver/BTDLinearSolver.cpp 
    linearsolver/FullVector.cpp 
    linearsolver/GraphScatteredTypes.cpp 
    linearsolver/DefaultMultiMatrixAccessor.cpp 
    linearsolver/MatrixLinearSolver.cpp 
    linearsolver/SingleMatrixAccessor.cpp

    )
    
add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})

set(COMPILE_DEFINES "SOFA_BUILD_BASE_LINEAR_SOLVER")
set(LINK_DEPENDENCIES SofaSimulationTree)

include(${SOFA_CMAKE_DIR}/post.cmake)
