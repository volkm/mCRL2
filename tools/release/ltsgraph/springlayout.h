// Author(s): Rimco Boudewijns and Sjoerd Cranen
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/**

  @file springlayout.h
  @author S. Cranen, R. Boudewijns

  This file contains an implementation and user interface which enables automatic positioning for a graph.

*/

#ifndef SPRINGLAYOUT_H
#define SPRINGLAYOUT_H

#include <QDockWidget>
#include "ui_springlayout.h"
#include <QtOpenGL>

#include "glwidget.h"
#include "layoututility.h"

namespace Graph
{
class SpringLayoutUi;

class SpringLayout
{
  public:

    /**
     * @brief An enumeration that identifies the types of spring types which can be selected.
     */
    enum AttractionCalculation
    {
      ltsgraph,                   ///< LTSGraph implementation.
      linearsprings,              ///< Linear spring implementation.
      electricalsprings,          ///< LTSGraph implementation using approximated repulsion forces.
    };

    enum RepulsionCalculation
    {
      ltsgraph_repulsion,
      electricalsprings_repulsion,
      no_repulsion
    };

    enum ForceApplication
    {
      ltsgraph_application, ///< Treat forces as speed
      force_directed_application, ///< Treat forces as suggested direction
      force_directed_annealing_application, ///< Include annealing for speed
      force_cumulative,
    };
  private:
    AttractionCalculation m_option_attractionCalculation;
    RepulsionCalculation m_option_repulsionCalculation;
    ForceApplication m_option_forceApplication;

    std::size_t m_max_num_nodes = 0;
    std::size_t m_total_num_nodes = 0;

    Octree m_node_tree;
    Octree m_handle_tree;
    Octree m_trans_tree;

    // UI parameters
    const float m_min_speed = 0.00001f;
    const float m_max_speed = 5.0f;
    float m_speed;                   ///< The rate of change each step.
    const float m_min_attraction = 0.0f;
    const float m_max_attraction = 0.05f;
    float m_attraction;              ///< The attraction of the edges.
    const float m_min_repulsion = 0.0f;
    const float m_max_repulsion = 100.0f;
    float m_repulsion;               ///< The repulsion of other nodes.
    const float m_min_natLength = 0.0f;   
    const float m_max_natLength = 100.0f;
    float m_natLength;               ///< The natural length of springs.
    const float m_min_controlPointWeight = 0.0f;
    const float m_max_controlPointWeight = 1.0f;
    float m_controlPointWeight;      ///< The handle repulsion wight factor.
    const float m_min_accuracy = 5.0f;
    const float m_max_accuracy = 0.0f;
    float m_accuracy;                ///< Controls the Barnes-Hut criterion in the approximation of repulsive forces
    bool m_tree_enabled;
    std::vector<QVector3D> m_nforces, m_hforces, m_lforces, m_sforces;  ///< Vector of the calculated forces..

    // Annealing parameters
    const int anneal_iterations = 10;
    const float anneal_multiplier = 0.7f;
    float m_anneal_speed = 1.0f;

    Graph& m_graph;               ///< The graph on which the algorithm is applied.
    SpringLayoutUi* m_ui;         ///< The user interface generated by Qt.

    QVector3D(SpringLayout::*m_attractionCalculation)(const QVector3D&, const QVector3D&, float);
    QVector3D(SpringLayout::*m_repulsionCalculation)(const QVector3D&, const QVector3D&, float, float);
    void(SpringLayout::*m_forceApplication)(QVector3D&, const QVector3D&, float);

    /**
     * @brief Calculate the force of a linear spring between @e a and @e b.
     * @param a The first coordinate.
     * @param b The second coordinate.
     * @param ideal The ideal distance between @e a and @e b.
     */
    const float m_spring_constant = 1e-4f;
    QVector3D forceLinearSprings(const QVector3D& a, const QVector3D& b, float ideal);

    /**
     * @brief Calculate the force of a LTSGraph "spring" between @e a and @e b.
     * @param a The first coordinate.
     * @param b The second coordinate.
     * @param ideal The ideal distance between @e a and @e b.
     */
    QVector3D forceLTSGraph(const QVector3D& a, const QVector3D& b, float ideal);

    /**
     * @brief Calculate the force of a spring modelled as electrical particles between @e a and @e b.
     * 
     * @param a The first coordinate.
     * @param b The second coordinate.
     * @param natlength The natural (preferred) distance between @e a and @e b.
     */
    QVector3D forceSpringElectricalModel(const QVector3D& a, const QVector3D& b, float natlength);

    /**
     * @brief returns repulsion force as defined by ltsgraph
     * 
     * @param a Particle position
     * @param b Other particle position
     * @param repulsion Scaling constant 
     * @param natlength Other scaling constant
     * @return QVector3D Force exerted by particle b on particle a
     */
    QVector3D repulsionForceLTSGraph(const QVector3D& a, const QVector3D& b, float repulsion, float natlength);

    /**
     * @brief returns repulsion force as defined by electrical spring model
     * 
     * @param a Particle position
     * @param b Other particle position
     * @param repulsion Scaling constant 
     * @param natlength Other scaling constant
     * @return QVector3D Force exerted by particle @e b on particle @e a
     */
    QVector3D repulsionForceElectricalModel(const QVector3D& a, const QVector3D& b, float repulsion, float natlength);

    /**
     * @brief Used to disable repulsion force
     * 
     */
    QVector3D noRepulsionForce(const QVector3D& a, const QVector3D& b, float repulsion, float natlength) { return {0 , 0, 0};}


    /**
     * @brief Returns approximate accumulation of all repulsive forces from other particles exerted on @e a
     * 
     * @param a Particle
     * @param tree Octree containing all particles
     * @param repulsion Scaling constant
     * @param natlength Other scaling constant
     * @return QVector3D Force exerted by all particles on particle @e a
     */
    QVector3D approxRepulsionForce(const QVector3D& a, Octree& tree, float repulsion, float natlength);

    /**
     * @brief Applies accumulated forces directly to the positions
     * 
     */
    void applyForceLTSGraph(QVector3D& pos, const QVector3D& force, float speed);

    /**
     * @brief Applies direction of force times speed
     * 
     * @param pos Current position of particle to be updated
     * @param force Accumulated force
     * @param speed User defined speed variable
     */
    void applyForceDirected(QVector3D& pos, const QVector3D& force, float speed);
  public:
    GLWidget& m_glwidget;

    /**
     * @brief Constructor of the algorithm for the given @e graph.
     * @param graph The graph on which the algorithm should be applied.
     */
    SpringLayout(Graph& graph, GLWidget& glwidget);

    /**
     * @brief Destructor.
     */
    virtual ~SpringLayout();

    /**
     * @brief Calculate the forces and update the positions.
     */
    void apply();

    /**
     * @brief Set the type of the force calculation.
     * @param c The desired calculaten 
     */
    void setAttractionCalculation(AttractionCalculation c);

    /**
     * @brief Returns the current force calculation used.
     */
    AttractionCalculation attractionCalculation();

    /**
     * @brief Set the type of the force calculation.
     * @param c The desired calculaten 
     */
    void setRepulsionCalculation(RepulsionCalculation c);

    /**
     * @brief Returns the current repulsion calculation used.
     */
    RepulsionCalculation repulsionCalculation();

    /**
     * @brief Set the type of force application.
     * @param c The desired way force will be applied to the graphs elements
     */
    void setForceApplication(ForceApplication c);

    /**
     * @brief Returns the current force application method.
     */
    ForceApplication forceApplication();

    /**
     * @brief Randomly moves nodes along the Z axis, at most [z] units
     * @param z The maximum distance that nodes are moved
     */
    void randomizeZ(float z);

    /**
     * @brief Pass-through whether graph is stable or not
     */
    const bool& isStable(){ return m_graph.stable(); };

    /**
     * @brief Returns the user interface object. If no user interface is available,
     *        one is created using the provided @e parent.
     * @param The parent of the user inferface in the case none exists yet.
     */
    SpringLayoutUi* ui(QWidget* parent = nullptr);

    //Getters and setters
    float lerp(int value, float targ_min, float targ_max, int _min = 0,
               int _max = 100)
    {
      return targ_min +
             (targ_max - targ_min) * (value - _min) / (float)(_max - _min);
    }
    int unlerp(float value, float val_min, float val_max, int targ_min = 0,
               int targ_max = 100) const
    {
      return targ_min +
             (targ_max - targ_min) * (value - val_min) / (val_max - val_min);
    }
    int speed() const {
      return unlerp(m_speed, m_min_speed, m_max_speed);
    }
    int attraction() const {
      return unlerp(m_attraction, m_min_attraction, m_max_attraction);
    }
    int repulsion() const {
      float cube = m_natLength * m_natLength * m_natLength;
      return unlerp(m_repulsion / cube, m_min_repulsion, m_max_repulsion);
    }
    int controlPointWeight() const {
      return unlerp(m_controlPointWeight, m_min_controlPointWeight, m_max_controlPointWeight);
    }
    int naturalTransitionLength() const {
      return unlerp(m_natLength, m_min_natLength, m_max_natLength);
    }

    bool treeEnabled() const {
      return m_tree_enabled;
    }

    void setTreeEnabled(bool b);
    void setSpeed(int v);
    void setAccuracy(int v);
    void setAttraction(int v);
    void setRepulsion(int v);
    void setControlPointWeight(int v);
    void setNaturalTransitionLength(int v);

    /// @brief Used to invalidate graph when settings change (i.e. attraction/repulsion)
    void rulesChanged();
};

class SpringLayoutUi : public QDockWidget
{
    Q_OBJECT
  private:
    SpringLayout& m_layout;     ///< The layout algorithm that corresponds to this user interface.
    QThread* m_thread;          ///< The thread that is used to calculate the new positions.
  public:
    Ui::DockWidgetLayout m_ui;  ///< The user interface generated by Qt.

    /**
     * @brief Constructor.
     * @param layout The layout object this user interface corresponds to.
     * @param parent The parent widget for this user interface.
     */
    SpringLayoutUi(SpringLayout& layout, QWidget* parent=nullptr);

    /**
     * @brief Destructor.
     */
    ~SpringLayoutUi() override;

    /**
     * @brief Get the current state of the settings.
     */
    QByteArray settings();

    /**
     * @brief Restore the settings of the given state.
     * @param state The original state
     */
    void setSettings(QByteArray state);

    /**
     * @brief Indicates that settings changed that influence the layout.
     * 
     */
    void layoutRulesChanged();
  signals:

    /**
     * @brief Indicates that the thread is started or stopped.
     */
    void runningChanged(bool);

  public slots:

    /**
     * @brief Updates the attraction value.
     * @param value The new value.
     */
    void onAttractionChanged(int value);

    /**
     * @brief Updates the repulsion value.
     * @param value The new value.
     */
    void onRepulsionChanged(int value);

    /**
     * @brief Updates the speed value.
     * @param value The new value.
     */
    void onSpeedChanged(int value);

    /**
     * @brief Updates accuracy value.
     * @param value New value.
     */
    void onAccuracyChanged(int value);

    /**
     * @brief Updates the handle weight.
     * @param value The new weight.
     */
    void onHandleWeightChanged(int value);

    /**
     * @brief Updates the natural length value.
     * @param value The new value.
     */
    void onNatLengthChanged(int value);

    /**
     * @brief Updates the attraction force calculation.
     * @param value The new index selected.
     */
    void onAttractionCalculationChanged(int value);

    /**
     * @brief Updates the repulsion force calculation.
     * @param value The new index selected.
     */
    void onRepulsionCalculationChanged(int value);

    /**
     * @brief Updates the force application.
     * @param value The new index selected.
     * 
     */
    void onForceApplicationChanged(int value);

    /**
     * @brief Starts or stops the force calculation depending on the current state.
     */
    void onStartStop();

    /**
     * @brief Starts the force calculation.
     */
    void onStarted();

    /**
     * @brief Stops the force calculation.
     */
    void onStopped();

    /**
     * @brief Enables/disables tree for acceleration
     */
    void onTreeToggled(bool);

    /**
     * @brief Starts or stops the force calculation.
     * @param active The calculation is started if this argument is true.
     */
    void setActive(bool active);
};
}  // namespace Graph

#endif // SPRINGLAYOUT_H
