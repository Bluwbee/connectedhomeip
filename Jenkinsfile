pipeline
{
    agent any
    stages
    {
        stage("Building")
        {
            agent { docker { image "ubuntu:latest" } }
            steps
            {
                sh echo "Hello You"
            }
        }
    }
 } // pipeline