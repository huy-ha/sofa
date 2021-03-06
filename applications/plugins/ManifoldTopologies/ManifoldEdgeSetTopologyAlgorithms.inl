/******************************************************************************
*                 SOFA, Simulation Open-Framework Architecture                *
*                    (c) 2006 INRIA, USTL, UJF, CNRS, MGH                     *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef SOFA_MANIFOLD_TOPOLOGY_EDGESETTOPOLOGYALGORITHMS_INL
#define SOFA_MANIFOLD_TOPOLOGY_EDGESETTOPOLOGYALGORITHMS_INL

#include <ManifoldTopologies/ManifoldEdgeSetTopologyAlgorithms.h>
#include <ManifoldTopologies/ManifoldEdgeSetTopologyContainer.h>
#include <ManifoldTopologies/ManifoldEdgeSetTopologyModifier.h>
#include <ManifoldTopologies/ManifoldEdgeSetGeometryAlgorithms.h>

namespace sofa
{

namespace component
{

namespace topology
{
using namespace sofa::defaulttype;
using namespace sofa::core::behavior;

template<class DataTypes>
void ManifoldEdgeSetTopologyAlgorithms< DataTypes >::init()
{
    EdgeSetTopologyAlgorithms< DataTypes >::init();
    this->getContext()->get(m_container);
    this->getContext()->get(m_modifier);
    this->getContext()->get(m_geometryAlgorithms);
}

} // namespace topology

} // namespace component

} // namespace sofa

#endif // SOFA_MANIFOLD_TOPOLOGY_EDGESETTOPOLOGYALGORITHMS_INL
