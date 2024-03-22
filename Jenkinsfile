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
                echo "Hello You"
            }
        }
    }
 } // pipeline