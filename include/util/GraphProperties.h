/** \file
 * \brief Implements a set of graph properties/metrics which are 
 * used by the benchmark system of the QGIS-Frontend.
 *
 * \author Tim Hartmann
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */
#pragma once

#include <ogdf/basic/SList.h>
#include <ogdf/graphalg/Dijkstra.h>
#include <ogdf/graphalg/ShortestPathAlgorithms.h>
#include <tuple>

namespace ogdf {

class GraphProperties
{
public:
    template <typename T>
    /**
     * Gives information about the fragility of the graph. The fragility of a
     * graph messures how much the shortest distances degrade if an edge is
     * deleted. The method returns a tuple containing the average, minimum and
     * maximum of all the fragilites.
     *
     * @param G The graph
     * @param weights The edge weights of the graph
     */
    std::tuple<double, T, T> fragility(const Graph &G, EdgeArray<T> &weights)
    {
        double avgFragility = 0;
        T maxFragility = 0;
        T minFragility = std::numeric_limits<T>::max();
        NodeArray<T> distResult;
        NodeArray<edge> preds;
        for (edge e : G.edges)
        {
            node x = e->nodes()[0];
            node y = e->nodes()[1];
            T edgeWeight = weights[e];
            weights[e] = std::numeric_limits<T>::max();
            Dijkstra<T>().callBound(G, weights, x, preds, distResult, false, false, y,
                                    std::numeric_limits<T>::max());
            T distWithoutEdge = distResult[y];
            weights[e] = edgeWeight;
            Dijkstra<T>().callBound(G, weights, x, preds, distResult, false, false, y,
                                    std::numeric_limits<T>::max());
            T distWithEdge = distResult[y];
            double frag = (float(distWithoutEdge - distWithEdge) / distWithEdge) + 1;
            if (frag > maxFragility)
            {
                maxFragility = frag;
            }
            if (frag < minFragility)
            {
                minFragility = frag;
            }
            avgFragility += frag / G.numberOfEdges();
        }
        return std::make_tuple(avgFragility, maxFragility, minFragility);
    }

    template <typename T>
    /**
     * Method to calculate the graph radius which is defined as the 
     * minimum graph eccentricity of any vertex.
     *
     * @param G The graph
     * @param weights The edge weights of the graph     
     */
    T graphRadius(const Graph &G, const EdgeArray<T> &weights)
    {
        T radius = std::numeric_limits<T>::max();
        NodeArray<T> distResult;
        NodeArray<edge> preds;
        for (node n : G.nodes)
        {
            T maximumDistance = 0;
            Dijkstra<T>().call(G, weights, n, preds, distResult);
            for (node m : G.nodes)
            {
                if (distResult[m] > maximumDistance)
                {
                    maximumDistance = distResult[m];
                }
            }
            if (maximumDistance < radius)
            {
                radius = maximumDistance;
            }
        }
        return radius;
    }

    template <typename T>
    /**
     * Method to calculate the graph diameter which is defined as the
     * maximum of all point to point shortest distances.
     * @param G The graph
     * @param weights The edge weights of the graph
     */
    T graphDiameter(const Graph &G, const EdgeArray<T> &weights)
    {
        T diameter = 0;
        NodeArray<T> distResult;
        NodeArray<edge> preds;
        for (node n : G.nodes)
        {
            Dijkstra<T>().call(G, weights, n, preds, distResult);
            for (node m : G.nodes)
            {
                if (distResult[m] > diameter)
                {
                    diameter = distResult[m];
                }
            }
        }
        return diameter;
    }

    template <typename T>
    /**
     * The girth of the graph is the length of the shortest 
     * cycle. If called with unit weights it returns the
     * number of edges in this cycle.
     *
     * @param G The graph
     * @param weights The edge weights of the graph
     */
    T girth(const Graph &G, EdgeArray<T> &weights)
    {
        NodeArray<T> distResult;
        NodeArray<edge> preds;
        T girth = std::numeric_limits<T>::max();
        for (edge e : G.edges)
        {
            node x = e->nodes()[0];
            node y = e->nodes()[1];
            T edgeWeight = weights[e];
            weights[e] = std::numeric_limits<T>::max();
            Dijkstra<T>().callBound(G, weights, x, preds, distResult, false, false, y,
                                    std::numeric_limits<T>::max());
            weights[e] = edgeWeight;
            if (distResult[y] + edgeWeight < girth)
            {
                girth = distResult[y] + edgeWeight;
            }
        }
        return girth;
    }
};
}  // namespace ogdf