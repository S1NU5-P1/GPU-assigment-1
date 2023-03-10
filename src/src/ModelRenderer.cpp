#include "ModelRenderer.h"

#include "Nodes/ModelNode.h"
#include "Model.h"
#include "LoggingMacros.h"
#include "MainEngine.h"

void ModelRenderer::Draw(MainEngine* engine)
{
    for (auto& [Model, NodeArray] : nodesMap)
    {
        DrawModel(Model, NodeArray, engine);
    }
}

void ModelRenderer::DrawModel(Model* model, std::set<struct ModelNode*>& nodeArray, MainEngine* engine)
{
    for (auto& Node: nodeArray)
    {
        if (Node->WasDirtyThisFrame())
        {
            UpdateMatrixBuffer(model, nodeArray);
            break;
        }
    }

    model->GetShader()->Activate();

    for (const auto& Mesh : model->GetMeshes())
    {
        Mesh->BindTextures(*model->GetShader());

        if (engine && model->GetShader()->GetUniformLocation("cubemap") >= 0)
        {
            glActiveTexture(GL_TEXTURE0 + 15);
            model->GetShader()->SetInt("cubemap", 15);
            glBindTexture(GL_TEXTURE_CUBE_MAP, engine->GetSkyboxTextureId());
            glActiveTexture(GL_TEXTURE0);
        }

        glBindVertexArray(Mesh->GetVao().GetVaoId());
        glDrawElementsInstanced(GL_TRIANGLES, Mesh->GetVao().GetIndicesCount(), GL_UNSIGNED_INT, 0, nodeArray.size());
        glBindVertexArray(0);
    }
}

void ModelRenderer::UpdateMatrixBuffer(Model* model, std::set<ModelNode*>& nodeArray)
{
    std::vector<glm::mat4> Matrices;
    for (ModelNode* Node : nodeArray)
    {
        Matrices.push_back(*Node->GetWorldTransformMatrix());
    }

    GLuint Buffer = bufferMap[model];
    glBindBuffer(GL_ARRAY_BUFFER, Buffer);
    glBufferData(GL_ARRAY_BUFFER, Matrices.size() * sizeof(glm::mat4), &Matrices[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ModelRenderer::AddNode(ModelNode* node)
{
    nodesMap[node->GetModel()].insert(node);
    if (bufferMap.find(node->GetModel()) == bufferMap.end())
    {
        unsigned int MatrixBuffer;
        glGenBuffers(1, &MatrixBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, MatrixBuffer);

        bufferMap[node->GetModel()] = MatrixBuffer;

        for (const auto& Mesh : node->GetModel()->GetMeshes())
        {
            glBindVertexArray(Mesh->GetVao().GetVaoId());
            GLsizei SizeOfVec4 = sizeof(glm::vec4);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * SizeOfVec4, (void*)0);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * SizeOfVec4, (void*)(SizeOfVec4));
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * SizeOfVec4, (void*)(2 * SizeOfVec4));
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * SizeOfVec4, (void*)(3 * SizeOfVec4));

            glVertexAttribDivisor(3, 1);
            glVertexAttribDivisor(4, 1);
            glVertexAttribDivisor(5, 1);
            glVertexAttribDivisor(6, 1);

            glBindVertexArray(0);
        }
    }
}

void ModelRenderer::RemoveNode(ModelNode* node)
{
    auto NodesSet = nodesMap[node->GetModel()];
    NodesSet.erase(node);
    if (NodesSet.empty())
    {
        nodesMap.erase(node->GetModel());
        GLuint Buffer = bufferMap[node->GetModel()];
        glDeleteBuffers(1, &Buffer);
        bufferMap.erase(node->GetModel());
    }
}
